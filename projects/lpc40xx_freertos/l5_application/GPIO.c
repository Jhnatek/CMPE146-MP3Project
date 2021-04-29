
#include "gpio_lab.h"
#include "lpc40xx.h"
#include <stdbool.h>
#include <stdint.h>
void gpio0__set_as_input(uint8_t pin_num) { LPC_GPIO0->DIR &= ~(1 << pin_num); }
void gpio0__set_as_output(uint8_t pin_num) { LPC_GPIO0->DIR |= (1 << pin_num); }
void gpio0__set_high(uint8_t pin_num) { LPC_GPIO0->PIN |= (1 << pin_num); }
void gpio0__set_low(uint8_t pin_num) { LPC_GPIO0->PIN &= ~(1 << pin_num); }
void gpio0_set(uint8_t pin_num, bool high) {
  if (high)
    gpio0__set_high(pin_num);
  else
    gpio0__set_low(pin_num);
}
bool gpio0__get_level(uint8_t pin_num) {
  if (LPC_GPIO0->PIN & (1 << pin_num))
    return true;
  else
    return false;
}
void gpio1__set_as_input(uint8_t pin_num) { LPC_GPIO1->DIR &= ~(1 << pin_num); }
void gpio1__set_as_output(uint8_t pin_num) { LPC_GPIO1->DIR |= (1 << pin_num); }
void gpio1__set_high(uint8_t pin_num) { LPC_GPIO1->PIN |= (1 << pin_num); }
void gpio1__set_low(uint8_t pin_num) { LPC_GPIO1->PIN &= ~(1 << pin_num); }
void gpio1_set(uint8_t pin_num, bool high) {
  if (high)
    gpio1__set_high(pin_num);
  else
    gpio1__set_low(pin_num);
}
bool gpio1__get_level(uint8_t pin_num) {
  if (LPC_GPIO1->PIN & (1 << pin_num))
    return true;
  else
    return false;
}
