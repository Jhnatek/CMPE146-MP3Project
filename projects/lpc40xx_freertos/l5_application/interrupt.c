#include "lpc40xx.h"
#include "interrupt.h"

static function_pointer_t gpio0_callbacks[32];
static function_pointer_t gpio1_callbacks[32];
static function_pointer_t gpio2_callbacks[32];

void gpio__attach_interrupt(uint32_t port, uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (port == 0) {
    gpio0__attach_interrupt(pin, interrupt_type, callback);
  } 
  else if (port == 1) {
    gpio1__attach_interrupt(pin, interrupt_type, callback);
  }
  else if (port == 2) {
    gpio2__attach_interrupt(pin, interrupt_type, callback);
  }
}

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  gpio0_callbacks[pin] = callback;
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  } else {
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  }
}
void gpio1__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  gpio1_callbacks[pin] = callback;
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO1IntEnF |= (1 << pin);
  } else {
    LPC_GPIOINT->IO1IntEnR |= (1 << pin);
  }
}

void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  gpio2_callbacks[pin] = callback;
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    LPC_GPIOINT->IO2IntEnF |= (1 << pin);
  } else {
    LPC_GPIOINT->IO2IntEnR |= (1 << pin);
  }
}

int findPos(uint32_t status) {
  uint32_t i = 1;
  uint32_t pos = 0;

  while (!(i & status)) {
    i = i << 1;
    ++pos;
  }
  return pos;
}

int findPin(uint32_t statusFE, uint32_t statusRE) {
  if (statusFE == 0) {
    return findPos(statusRE);
  } else {
    return findPos(statusFE);
  }
}

void clear_pin_interrupt(uint32_t pin) { LPC_GPIOINT->IO0IntClr = (1 << pin); }

void gpio__interrupt_dispatcher(void) {
  if ((LPC_GPIOINT->IO0IntStatF != 0) || (LPC_GPIOINT->IO0IntStatR != 0)) {
      gpio0__interrupt_dispatcher();
  } 
  else if ((LPC_GPIOINT->IO1ntStatF != 0) || (LPC_GPIOINT->IO1IntStatR != 0)) {
      gpio1__interrupt_dispatcher();
  }
  else {
     gpio2__interrupt_dispatcher();
  }
}

void gpio0__interrupt_dispatcher(void) {
  const int pin_that_generated_interrupt = findPin(LPC_GPIOINT->IO0IntStatF, LPC_GPIOINT->IO0IntStatR);
  function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];

  attached_user_handler();
  clear_pin_interrupt(pin_that_generated_interrupt);
}

void gpio0__interrupt_dispatcher(void) {
  const int pin_that_generated_interrupt = findPin(LPC_GPIOINT->IO1IntStatF, LPC_GPIOINT->IO1IntStatR);
  function_pointer_t attached_user_handler = gpio1_callbacks[pin_that_generated_interrupt];

  attached_user_handler();
  clear_pin_interrupt(pin_that_generated_interrupt);
}

void gpio2__interrupt_dispatcher(void) {
  const int pin_that_generated_interrupt = findPin(LPC_GPIOINT->IO2IntStatF, LPC_GPIOINT->IO2IntStatR);
  function_pointer_t attached_user_handler = gpio2_callbacks[pin_that_generated_interrupt];

  attached_user_handler();
  clear_pin_interrupt(pin_that_generated_interrupt);
}