#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "bsp/board.h"
#include "hardware/uart.h"
#include <hardware/gpio.h>

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_PIN_TX 0
#define UART_PIN_RX 1
#define UART_IRQ_HANDLER uart0_irq_handler
#define UART_BUFFER_SIZE 128

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

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task();
void uart_task();

/*------------- MAIN -------------*/
int main(void)
{
    board_init();
    tusb_init();

    // uart intialization
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, false);

    tud_init(BOARD_TUD_RHPORT);

    uart_puts(UART_ID, "Initalization complete.\n");
    while (1)
    {
        tud_task();
        led_blinking_task();
        uart_task();
        // hid_task();
    }
    return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

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

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

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

//--------------------------------------------------------------------+
// UART TASK
//--------------------------------------------------------------------+
void process_command(const char *command)
{
    uart_puts(UART_ID, "Received command: ");
    uart_puts(UART_ID, command);
    uart_puts(UART_ID, "\n");

    // Remote wakeup
    if (tud_suspended())
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }

    if (strcmp(command, "click") == 0)
    {
        // Mouse click
        if (tud_hid_n_ready(ITF_MOUSE))
        {
            tud_hid_n_mouse_report(ITF_MOUSE, 0, MOUSE_BUTTON_LEFT, 0, 0, 0, 0);
        }
    }
    else if (strcmp(command, "release") == 0)
    {
        if (tud_hid_n_ready(ITF_MOUSE))
        {
            tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, 0, 0, 0, 0);
        }
    }
    else if (strncmp(command, "move,", 5) == 0)
    {
        // Parse move command
        int dx, dy;
        if (sscanf(command + 5, "%d,%d", &dx, &dy) == 2)
        {
            // Move mouse with specified delta values
            if (tud_hid_n_ready(ITF_MOUSE))
            {
                tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, dx, dy, 0, 0);
            }
        }
    }
}

void uart_task()
{
    // poll every 10 ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms)
        return; // not enough time
    start_ms += interval_ms;

        if (uart_is_readable(UART_ID))
    {
        char c = uart_getc(UART_ID);
        uart_putc(UART_ID, c); // Echo back the character
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
