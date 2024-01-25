#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "bsp/board.h"
#include "usb_descriptors.h"
#include "hardware/uart.h"
#include <hardware/gpio.h>


#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_PIN_TX 4
#define UART_PIN_RX 5

enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(uint8_t keystroke);
uint8_t uart_task(void);


int main(void)
{
  board_init();
  tusb_init();
  
  uart_init(UART_ID, BAUD_RATE);

  gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);

  while (1)
  {
    tud_task();
    led_blinking_task();

    uint8_t keystroke = uart_task();

    hid_task(keystroke);
  }
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
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}


static void send_hid_report(uint8_t report_id, uint8_t keystroke)
{
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      static bool has_keyboard_key = false;

      if ( keystroke != 0)
      {
        uint8_t keycode[6] = { 0 };
        // keycode[0] = HID_KEY_A;
        keycode[0] = keystroke;
        keystroke = 0;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
      int8_t const delta = 5;

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, MOUSE_BUTTON_BACKWARD, delta, delta, 0, 0);
    }
    break;

    default: break;
  }
}

void hid_task(uint8_t keystroke)
{
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  if ( tud_suspended() && keystroke != 0 )
  {
    tud_remote_wakeup();
  }else
  {
    send_hid_report(REPORT_ID_KEYBOARD, keystroke);
  }
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}


void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  if (!blink_interval_ms) return;

  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

uint8_t uart_task(void)
{
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return 0; // not enough time
  start_ms += interval_ms;

  uint8_t keystroke = 0;

  if (uart_is_readable(UART_ID)) {
    char keystroke_char = uart_getc(UART_ID);
    keystroke = (uint8_t) keystroke_char;
  }

  return keystroke;
}