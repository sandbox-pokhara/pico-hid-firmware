#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "hardware/uart.h"
#include <hardware/gpio.h>

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_PIN_TX 0
#define UART_PIN_RX 1
#define START_CHARACTER '~'
#define END_CHARACTER '$'
#define UART_BUFFER_SIZE 50

#define UART_IRQ_HANDLER uart0_irq_handler

char uart_rx_buffer[UART_BUFFER_SIZE];
int buffer_index = 0;

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

#define MAX_KEYS 5 // Maximum number of keys that can be pressed at once

struct HID_FORMAT
{
    char keystroke;
    char keys_pressed[MAX_KEYS]; // Array representing the list of pressed keys
    uint8_t key_index;           // Number of keys currently pressed
    uint8_t button;
    uint8_t button_pressed;
};

struct HID_FORMAT hid_report = {0, {0}, 0, 0, 0};

uint8_t const conv_table[128][2] = {HID_ASCII_TO_KEYCODE};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task();
void on_uart_rx();
void button_debug_task(void);
void process_command(const char *command);
void ascii_to_hid(char ascii_char, uint8_t *modifier, uint8_t *keycode);

int main(void)
{
    board_init();
    tusb_init();

    // UART Intialization
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

    //-------------------------------------------------------------//

    tud_init(BOARD_TUD_RHPORT);

    uart_puts(UART_ID, "Initialization complete.\n");

    while (1)
    {
        tud_task();
        led_blinking_task();
        hid_task();
        button_debug_task();
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

void hid_task()
{
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time
    start_ms += interval_ms;

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    if (tud_hid_n_ready(ITF_KEYBOARD))
    {
        static uint8_t prev_keycode[6] = {0}; // Array representing the list of pressed keys and the last keystroke

        uint8_t modifier = 0;
        uint8_t keycode_hid = 0;

        // Assign hid_report.keys_pressed to the keycode array
        uint8_t keycode[6] = {0};
        for (int i = 0; i < hid_report.key_index; i++)
        {
            ascii_to_hid(hid_report.keys_pressed[i], &modifier, &keycode_hid);
            keycode[i] = keycode_hid;
        }

        ascii_to_hid(hid_report.keystroke, &modifier, &keycode_hid);
        keycode[hid_report.key_index] = keycode_hid;

        hid_report.keystroke = 0; // Clear the keystroke

        bool report_changed = false;
        // Check if the report has changed
        for (int i = 0; i < 6; i++)
        {
            if (keycode[i] != prev_keycode[i])
            {
                report_changed = true;
                break;
            }
        }

        if (report_changed)
        {
            tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, modifier, keycode);
            // Update previous report
            memcpy(prev_keycode, keycode, 6);
        }
    }

    if (tud_hid_n_ready(ITF_MOUSE))
    {
        static uint8_t prev_mouse_button = 0x00;

        if (hid_report.button != prev_mouse_button)
        {
            tud_hid_n_mouse_report(ITF_MOUSE, 0, hid_report.button, 0, 0, 0, 0);
            prev_mouse_button = hid_report.button;
        }

        if (hid_report.button_pressed == 1)
        {
            hid_report.button = MOUSE_BUTTON_LEFT;
        }
        else if (hid_report.button_pressed == 2)
        {
            hid_report.button = MOUSE_BUTTON_RIGHT;
        }
        else
        {
            hid_report.button = 0x00;
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

void button_debug_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time
    start_ms += interval_ms;

    uint32_t const btn = board_button_read();
    static bool has_key = false;

    // If USB is suspended and a button is pressed, wake up the device
    if (tud_suspended() && btn)
    {
        tud_remote_wakeup();
    }

    // If a button is pressed
    if (btn)
    {
        // If keyboard HID interface is ready
        if (tud_hid_n_ready(ITF_KEYBOARD))
        {
            uint8_t keycode[6] = {0};
            keycode[0] = HID_KEY_A; // Sending 'A' key
            tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, keycode);
            has_key = true;
        }

        // If mouse HID interface is ready
        if (tud_hid_n_ready(ITF_MOUSE))
        {
            int8_t pos = 5;
            tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, pos, pos, 0, 0);
        }
    }
    else
    {
        // If there was previously a key pressed, send an empty key report
        if (has_key)
        {
            tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, NULL);
        }
        has_key = false;
    }
}

void on_uart_rx()
{
    while (uart_is_readable(UART_ID))
    {
        char c = uart_getc(UART_ID);
        if (uart_is_writable(UART_ID))
        {
            uart_putc(UART_ID, c);
            if (c == '\r' || c == '\n' || buffer_index >= UART_BUFFER_SIZE - 1)
            {
                uart_rx_buffer[buffer_index] = '\0'; // Null-terminate the string
                if (buffer_index > 0)
                {
                    // Process the received command
                    process_command(uart_rx_buffer);
                }
                buffer_index = 0; // Reset buffer index
            }
            else
            {
                uart_rx_buffer[buffer_index++] = c;
            }
        }
    }
}
void process_command(const char *command)
{
    if (strcmp(command, "mouse_click_left") == 0)
    {
        hid_report.button = MOUSE_BUTTON_LEFT;
        hid_report.button_pressed = false;
    }
    else if (strcmp(command, "mouse_click_right") == 0)
    {
        hid_report.button = MOUSE_BUTTON_RIGHT;
        hid_report.button_pressed = false;
    }
    else if (strcmp(command, "mouse_press_left") == 0)
    {
        hid_report.button_pressed = 1;
    }
    else if (strcmp(command, "mouse_press_right") == 0)
    {
        hid_report.button_pressed = 2;
    }
    else if (strcmp(command, "mouse_release") == 0)
    {
        hid_report.button_pressed = 0;
    }
    else if (strncmp(command, "mouse_move,", 11) == 0)
    {
        // Parse mouse move command
        int16_t dx, dy;
        if (sscanf(command + 11, "%hd,%hd", &dx, &dy) == 2)
        {
            tud_hid_n_mouse_report(ITF_MOUSE, 0, 0, dx, dy, 0, 0);
        }
    }
    else if (strncmp(command, "keyboard_keystroke,", 19) == 0)
    {
        // Parse keyboard keystroke command
        char code;
        if (sscanf(command + 19, "%c", &code) == 1)
        {
            hid_report.keystroke = code;
        }
    }
    else if (strncmp(command, "keyboard_press,", 15) == 0)
    {
        // Parse keyboard press command
        char code;
        if (sscanf(command + 15, "%c", &code) == 1)
        {
            if (hid_report.key_index < MAX_KEYS)
            {
                hid_report.keys_pressed[hid_report.key_index++] = code;
            }
        }
    }
    else if (strncmp(command, "keyboard_release,", 17) == 0)
    {
        char code;
        if (sscanf(command + 17, "%c", &code) == 1)
        {
            for (int i = 0; i < hid_report.key_index; i++)
            {
                if (hid_report.keys_pressed[i] == code)
                {
                    // Shift elements to remove released key
                    for (int j = i; j < hid_report.key_index; j++)
                    {
                        hid_report.keys_pressed[j] = hid_report.keys_pressed[j + 1];
                    }
                    hid_report.key_index--;
                    break;
                }
            }
        }
    }
    else if (strcmp(command, "keyboard_release") == 0)
    {
        hid_report.key_index = 0;
        memset(hid_report.keys_pressed, 0, MAX_KEYS); // Clear keys_pressed array
    }
}

void ascii_to_hid(char ascii_char, uint8_t *modifier, uint8_t *keycode)
{
    *keycode = 0;
    // Check if ASCII character is within valid range
    if (ascii_char > 0 && ascii_char < 128)
    {
        // Check if left shift modifier is needed
        if (conv_table[ascii_char][0])
        {
            *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        }
        // Get the HID keycode
        *keycode = conv_table[ascii_char][1];
    }
}
