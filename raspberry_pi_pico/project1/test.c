#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"

#include "tusb.h"
#include "bsp/board.h"
#include "usb_descriptors.h"
#include "hardware/uart.h"
#include <hardware/gpio.h>

#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_PIN_TX 4
#define UART_PIN_RX 5

#define LED_PIN 25

void hid_task(uint8_t keystroke);
uint8_t uart_task(void);

int main()
{
    board_init();
    tusb_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    while (true)
    {
        tud_task();

        // read button state
          uint32_t const btn = board_button_read();

        uint8_t keystroke = uart_task();
        
        hid_task(keystroke);

        if (keystroke != 0)
        {
            gpio_put(LED_PIN, 1);
            sleep_ms(250);
            gpio_put(LED_PIN, 0);
            // sleep_ms(250); // Removed unnecessary sleep here
        }
        else
        {
            gpio_put(LED_PIN, 0);
        }
    }
}

static void send_hid_report(uint8_t report_id, uint8_t keystroke)
{
    if (!tud_hid_ready())
        return;

    switch (report_id)
    {
    case REPORT_ID_KEYBOARD:
    {
        static bool has_keyboard_key = false;

        if (keystroke != 0)
        {
            uint8_t keycode[6] = {0};
            keycode[0] = HID_KEY_A;
            // keycode[0] = keystroke;
            keystroke = 0;

            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
            has_keyboard_key = true;
        }
        else
        {
            if (has_keyboard_key)
                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
            has_keyboard_key = false;
        }
    }
    break;
    default:
        break;
    }
}

void hid_task(uint8_t keystroke)
{
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time
    start_ms += interval_ms;

    if (tud_suspended() && keystroke != 0)
    {
        tud_remote_wakeup();
    }
    else
    {
        send_hid_report(REPORT_ID_KEYBOARD, keystroke);
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;

    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
        if (report_id == REPORT_ID_KEYBOARD)
        {
            // bufsize should be (at least) 1
            if (bufsize < 1)
                return;

            uint8_t const kbd_leds = buffer[0];

            if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
            {
                board_led_write(true);
            }
            else
            {
                board_led_write(false);
            }
        }
    }
}

uint8_t uart_task(void)
{
    uint8_t keystroke = 0;

    while (uart_is_readable(UART_ID))
    {
        char keystroke_char = uart_getc(UART_ID);
        keystroke = (uint8_t)keystroke_char;
    }

    return keystroke;
}
