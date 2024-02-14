#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "bsp/board.h"
#include "hardware/uart.h"
#include <hardware/gpio.h>

#define UART_ID uart1
#define DEBUG_UART_ID uart0
#define BAUD_RATE 115200

#define UART_PIN_TX 4
#define UART_PIN_RX 5

#define DEBUG_UART_PIN_TX 0
#define DEBUG_UART_PIN_RX 1

#define MESSAGE_LENGTH 31

enum
{
    ITF_KEYBOARD = 0,
    ITF_MOUSE = 1
};

enum
{
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

// "SMOU,0,0,-222,-222,-222,-222,1E" -> 31 chars
// "SKBD,A,0,50,50,0,0,99999999999E" -> 31 chars 
// in addition to 31 chars 1 char is added at the end for null terminator

struct UART_MESSAGE
{
    uint8_t report_id;
    uint8_t keystroke;
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t vertical;
    int8_t horizontal;
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(struct UART_MESSAGE *message);
bool uart_task(struct UART_MESSAGE *message);

int main(void)
{
    board_init();
    tusb_init();

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    uart_init(DEBUG_UART_ID, BAUD_RATE);

    gpio_set_function(DEBUG_UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(DEBUG_UART_PIN_RX, GPIO_FUNC_UART);

    tud_init(BOARD_TUD_RHPORT);

    while (1)
    {
        tud_task();
        led_blinking_task();

        struct UART_MESSAGE message = {ITF_KEYBOARD, 0, 0, 0, 0, 0, 0};

        if (uart_is_readable(UART_ID))
        {
            bool uart_complete = uart_task(&message);
        }

        hid_task(&message);
    }
    return 0;
}

void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

void hid_task(struct UART_MESSAGE *message)
{
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time
    start_ms += interval_ms;

    uint32_t const btn = board_button_read();

    if (tud_suspended() && (message->keystroke != 0 || btn))
    {
        tud_remote_wakeup();
    }
    /*------------- Keyboard -------------*/
    if (tud_hid_n_ready(ITF_KEYBOARD))
    {
        static bool has_key = false;

        if (message->keystroke != 0 || btn)
        {
            uint8_t keycode[6] = {0};
            keycode[0] = HID_KEY_A;
            // keycode[0] = message->keystroke;
            if (btn)
            {
                keycode[0] = HID_KEY_A;
                int8_t const delta = 5;
                tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, delta, delta, 0, 0);
                // add delay
                board_delay(10);
            }
            uart_puts(DEBUG_UART_ID, "button pressed!\n");

            tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, keycode);
            has_key = true;
        }
        else
        {
            if (has_key)
                tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, NULL);
            has_key = false;
        }
    }

    /*------------- Mouse -------------*/
    if (tud_hid_n_ready(ITF_MOUSE))
    {
        if (btn)
        {
            tud_hid_n_mouse_report(ITF_MOUSE, 0, message->buttons, message->x, message->y, message->vertical, message->horizontal);
        }
    }
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    // TODO set LED based on CAPLOCK, NUMLOCK etc...
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

void led_blinking_task(void)
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}

bool uart_task(struct UART_MESSAGE *message)
{
    char c = uart_getc(UART_ID);

    // Look for the start character 'S'
    if (c != 'S')
    {
        return false;
    }
    char receivedString[MESSAGE_LENGTH]; // 30 characters + 1 null terminator
    uart_read_blocking(UART_ID, (uint8_t *)receivedString, MESSAGE_LENGTH);

    if (receivedString[MESSAGE_LENGTH - 2] != 'E')
    {
        return false;
    }
    // Null-terminate the string
    receivedString[MESSAGE_LENGTH - 1] = '\0';
    receivedString[MESSAGE_LENGTH - 2] = '\0';

    char report_id[3];

    int filler;
    int buttons, x, y, vertical, horizontal;
    char keystroke;
    int parsed_fields = sscanf(receivedString, "%3s,%c,%d,%d,%d,%d,%d,%d", report_id, &keystroke, &buttons, &x, &y, &vertical, &horizontal, filler);

    if (parsed_fields == 8)
    {
        // Copy the message into the struct
        if (strcmp(report_id, "KBD") == 0)
        {
            message->report_id = ITF_KEYBOARD;
        }
        else if (strcmp(report_id, "MOU") == 0)
        {
            message->report_id = ITF_MOUSE;
        }
        message->keystroke = (uint8_t)keystroke;

        if (message->report_id == ITF_MOUSE)
        {
            message->keystroke = 0x01; // to enable tud_remote_wakeup()
        }

        message->buttons = buttons;
        message->x = x;
        message->y = y;
        message->vertical = vertical;
        message->horizontal = horizontal;

        return true;
    }

    return false;
}