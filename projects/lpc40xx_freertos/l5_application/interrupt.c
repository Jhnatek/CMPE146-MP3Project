#include "interrupt.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

static function_pointer_t gpio0_callbacks[32];
static function_pointer_t gpio2_callbacks[32];

void gpio__attach_interrupt(uint32_t port, uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (port == 0) {
    gpio0__attach_interrupt(pin, interrupt_type, callback);
  } else if (port == 2) {
    gpio2__attach_interrupt(pin, interrupt_type, callback);
  }
}

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  gpio_s port0_input = gpio__construct_as_input(0, pin); // constructs as input
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  } else {
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  }
  NVIC_EnableIRQ(GPIO_IRQn);
  gpio0_callbacks[pin] = callback;
}

void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  gpio_s port0_input = gpio__construct_as_input(GPIO__PORT_2, pin); // constructs as input
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO2IntEnF |= (1 << pin);
  } else {
    LPC_GPIOINT->IO2IntEnR |= (1 << pin);
  }
  NVIC_EnableIRQ(GPIO_IRQn);
  gpio2_callbacks[pin] = callback;
}

// int findPos(uint32_t status) {
//   uint32_t i = 1;
//   uint32_t pos = 0;

//   while (!(i & status)) {
//     i = i << 1;
//     ++pos;
//   }
//   return pos;
// }

// int findPin(uint32_t statusFE, uint32_t statusRE) {
//   if (statusFE == 0) {
//     return findPos(statusRE);
//   } else {
//     return findPos(statusFE);
//   }
// }

void clear_pin_interrupt(uint32_t pin) { LPC_GPIOINT->IO0IntClr = (1 << pin); }

void gpio__interrupt_dispatcher(void) {
  int pin_interrupt_found;
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO0IntStatF & (1 << i)) || (LPC_GPIOINT->IO0IntStatR & (1 << i))) {
      pin_interrupt_found = i;
      break;
    }
  }
  function_pointer_t attached_user_handler = gpio0_callbacks[pin_interrupt_found];
  attached_user_handler();
  clear_pin_interrupt(pin_interrupt_found);
}

// void gpio0__interrupt_dispatcher(void) {
//   const int pin_that_generated_interrupt = findPin(LPC_GPIOINT->IO0IntStatF, LPC_GPIOINT->IO0IntStatR);
//   function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];

//   attached_user_handler();
//   clear_pin_interrupt(pin_that_generated_interrupt);
// }

void gpio2__interrupt_dispatcher(void) {
  int pin_interrupt_found;
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO2IntStatF & (1 << i)) || (LPC_GPIOINT->IO2IntStatR & (1 << i))) {
      pin_interrupt_found = i;
      break;
    }
  }
  function_pointer_t attached_user_handler = gpio2_callbacks[pin_interrupt_found];
  attached_user_handler();
  clear_pin_interrupt(pin_interrupt_found);
}
