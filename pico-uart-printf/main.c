#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <string.h>

#define MESSAGE_LENGTH 31

#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_PIN_TX 4
#define UART_PIN_RX 5

enum
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE = 9,
    REPORT_ID_CONSUMER_CONTROL,
    REPORT_ID_GAMEPAD,
    REPORT_ID_COUNT
};

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

bool uart_task(struct UART_MESSAGE *message);

int main()
{
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    while (1)
    {
        struct UART_MESSAGE message = {REPORT_ID_KEYBOARD, 0, 0, 0, 0, 0, 0};

        if (uart_is_readable(UART_ID))
        {
            bool uart_complete = uart_task(&message);

            printf("Report ID: %d\n", message.report_id);
            printf("Keystroke: %d\n", message.keystroke);
            printf("Buttons: %d\n", message.buttons);
            printf("X: %d\n", message.x);
            printf("Y: %d\n", message.y);
            printf("Vertical: %d\n", message.vertical);
            printf("Horizontal: %d\n", message.horizontal);
        }
    }
}

bool uart_task(struct UART_MESSAGE *message)
{
    // const uint32_t interval_ms = 10;
    // static uint32_t start_ms = 0;

    // if (board_millis() - start_ms < interval_ms)
    //     return 0; // not enough time
    // start_ms += interval_ms;

    char c = uart_getc(UART_ID);

    // Look for the start character 's'
    if (c != 'S')
    {
        return false;
    }
    char receivedString[MESSAGE_LENGTH];
    uart_read_blocking(UART_ID, (uint8_t *)receivedString, MESSAGE_LENGTH);

    if (receivedString[MESSAGE_LENGTH - 2] != 'E')
    {
        return false;
    }
    // Null-terminate the string
    receivedString[MESSAGE_LENGTH - 2] = '\0';
    receivedString[MESSAGE_LENGTH - 1] = '\0';
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
            message->report_id = REPORT_ID_KEYBOARD;
        }
        else if (strcmp(report_id, "MOU") == 0)
        {
            message->report_id = REPORT_ID_MOUSE;
        }
        message->keystroke = (uint8_t)keystroke;

        if (message->report_id == REPORT_ID_MOUSE)
        {
            message->keystroke = 0;
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