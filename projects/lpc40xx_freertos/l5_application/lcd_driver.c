#include "lcd_driver.h"

#include <stdio.h>
#include <string.h>

#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "uart_lab.h"

const uint8_t lcd_columns = 20; // columns for the lcd
const uint8_t lcd_rows = 4;     // rows for the lcd

static const uint32_t lcd_initial_baud_rate = 9600; //

static Uart_Number_E Uart = UART__3;
static const uint8_t uart_tx_pin = 25; // transmit pin

void lcd_clear(void) {
  setting_command('-'); // clear the screen
}

void setting_command(char byte) {
  Uart_Driver__Polled_Put(Uart, 0x7C); // setting command signal
  delay__ms(10);
  Uart_Driver__Polled_Put(Uart, byte); // setting needed to change
  delay__ms(10);
}

void special_command(char byte) {
  Uart_Driver__Polled_Put(Uart, 254); // special command signal
  delay__ms(10);
  Uart_Driver__Polled_Put(Uart, byte); // special command
  delay__ms(50);
}

static void configure_uart_pin(void) {
  gpio_s u3_tx = gpio__construct_with_function(GPIO__PORT_0, uart_tx_pin, GPIO__FUNCTION_3);
  // gpio__set_as_output(u3_tx);
}

void lcd__initialize(void) {
  configure_uart_pin();
  uint32_t peripheral_clock_hz = clock__get_peripheral_clock_hz();
  Uart_Driver__Init(Uart, peripheral_clock_hz, lcd_initial_baud_rate);
  setting_command(0x08);        // reset lcd screen
  delay__ms(1000);              // wait 1000 ms
  special_command(0x08 | 0x04); // turn display on
  special_command(0x04 | 0x02); // set the entry mode
  lcd_clear();
  print_to_screen("the beatles");
}

void print_to_screen(char *str) {
  for (int i = 0; i < strlen(str); i++) {
    Uart_Driver__Polled_Put(Uart, str[i]); // send byte to uart pin
  }
}
