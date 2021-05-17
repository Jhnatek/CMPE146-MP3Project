#include "LED_control.h"
static void pwm_pin_configuration(uint32_t port, uint32_t pin) { gpio__construct_with_function(port, pin, 1); }

void initialize_pwm(void) {
  pwm1__init_single_edge(1000);
  for (int i = 0; i < 3; i++) {
    pwm_pin_configuration(2, i);
  }
}

void test_color(void) {
  pwm1__set_duty_cycle(PWM1__2_0, 50);
  pwm1__set_duty_cycle(PWM1__2_1, 50);
  pwm1__set_duty_cycle(PWM1__2_2, 50);
}