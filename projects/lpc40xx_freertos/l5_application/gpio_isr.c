#include "gpio_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacks[32];
// I did it in one b/c eventhough is rising edge triggered it would still call the same port's isr function.

void gpio0____attach_interrupt(
    uint32_t pin, gpio_interrupt_e interrupt_type,
    function_pointer_t callback) { // was previously gpio0__attach_interrupt (uint32_t pin, gpio_interrupt_e
                                   // interrupt_type, function_pointer_t callback)
  if (interrupt_type ==
      GPIO_INTR__FALLING_EDGE) // changed function name to gpio0____attach_interrupt(uint32_t pin, gpio_interrupt_e
                               // interrupt_type, function_pointer_t callback) bc had the same function in interrupts.c
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  else
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  gpio0_callbacks[pin] = callback;

  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
}

int logic_that_you_will_write() {
  /* --------Description of the implementation----------------
      1)Always check the rightmost bit is set
      2)Shift int_reg to right until it's detect the set bit and increment pin_num everytime int_reg is shifted
  */
  uint32_t int_reg_F = LPC_GPIOINT->IO0IntStatF, int_reg_R = LPC_GPIOINT->IO0IntStatR;
  int pin_num;
  for (pin_num = 0; (int_reg_F & 1) == 0 && (int_reg_R & 1) == 0; pin_num++) {
    if (pin_num >= 32)
      return -1;
    int_reg_F >>= 1;
    int_reg_R >>= 1;
  }
  fprintf(stderr, "Pin: %d\n", pin_num);
  return pin_num;
}

static void clear_pin_interrupt(const int pin_that_generated_interrupt) {
  LPC_GPIOINT->IO0IntClr = (1 << pin_that_generated_interrupt);
}
// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  const int pin_that_generated_interrupt = logic_that_you_will_write();
  if (pin_that_generated_interrupt == -1) {
    fprintf(stderr, "---Fail to Determine interrupt pin---\n");
    return;
  }
  function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
  clear_pin_interrupt(pin_that_generated_interrupt);
  // fprintf(stderr, "clr: %x", (int)LPC_GPIOINT->IO0IntClr);
}
