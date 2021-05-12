#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void (*function_pointer_t)(void);
typedef enum {
  GPIO_INTR__FALLING_EDGE,
  GPIO_INTR__RISING_EDGE,
} gpio_interrupt_e;

void gpio__attach_interrupt(uint32_t port, uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);
void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);
void gpio1__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);
void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback);
int findPos(uint32_t status);
int findPin(uint32_t statusFE, uint32_t statusRE);
void clear_pin_interrupt(uint32_t pin);
void gpio__interrupt_dispatcher(void);
void gpio0__interrupt_dispatcher(void);
void gpio0__interrupt_dispatcher(void);
void gpio2__interrupt_dispatcher(void);