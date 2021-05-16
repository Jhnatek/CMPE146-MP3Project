#pragma once
#include <stdint.h>

typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
void gpio0____attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);// // was previously gpio0__attach_interrupt (uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback)
// changed function name to gpio0____attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) bc had the same function in interrupts.c 
// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void gpio0__interrupt_dispatcher(void);
