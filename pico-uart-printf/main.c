#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define MESSAGE_LENGTH 29
#define TIMEOUT_US 1000000 // 1 second in microseconds

#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_PIN_TX 4
#define UART_PIN_RX 5

int main()
{
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

    while (1)
    {
        char receivedString[MESSAGE_LENGTH];

        // Look for the start character 's'
        char c = uart_getc(UART_ID);
        while (c != 'S')
        {
            printf("Waiting for start character\n");
            c = uart_getc(UART_ID);
        }

        // Read the rest of the message
        uart_read_blocking(UART_ID, (uint8_t *)receivedString, MESSAGE_LENGTH);
        
        if (receivedString[MESSAGE_LENGTH-2] != 'E')
        {
            printf("Message not terminated correctly\n");
            continue;
        }

        // Null-terminate the string
        receivedString[MESSAGE_LENGTH-2] = '\0';
        receivedString[MESSAGE_LENGTH-1] = '\0';

        // Print received message
        printf("Received message: %s\n", receivedString);

        // Assuming the message has the format "smou,buttons,x,y,vertical,horizontal"
        char key[3];
        char filler[6];
        int buttons, x, y, vertical, horizontal;
        int parsed_fields = sscanf(receivedString, "%3s,%d,%d,%d,%d,%d,%d", key, &buttons, &x, &y, &vertical, &horizontal, filler);

        if (parsed_fields == 7)
        {
            printf("Parsed message:\n");
            printf("Key: %s\n", key);
            printf("Buttons: %d\n", buttons);
            printf("X: %d\n", x);
            printf("Y: %d\n", y);
            printf("Vertical: %d\n", vertical);
            printf("Horizontal: %d\n", horizontal);
        }
        else
        {
            printf("Invalid message format\n");
        }
    }
}