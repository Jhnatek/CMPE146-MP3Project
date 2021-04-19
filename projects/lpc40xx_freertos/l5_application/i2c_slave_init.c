#include "i2c_slave_init.h"
#include "i2c.h"
#include "lpc40xx.h"
#include <stdio.h>

void i2c1__slave_init(uint8_t slave_address_to_respond_to) {
  i2c_1_peripheral_init();
  i2c_1_pin_init();
  LPC_I2C1->ADR0 = slave_address_to_respond_to;
  LPC_I2C1->CONSET = 0x44; // slave mode
}
void i2c2__master_init(uint8_t master_address_to_respond_to) {
  i2c_2_peripheral_init();
  LPC_I2C2->ADR0 = master_address_to_respond_to;
  LPC_I2C2->CONSET = 0x40; // master mode
}

static void i2c_1_peripheral_init(void) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz());

  for (unsigned slave_address = 2; slave_address <= 254; slave_address += 2) {
    if (i2c__detect(I2C__1, slave_address)) {
      printf("I2C slave detected at address: 0x%02X\n", slave_address);
    }
  }
}

static void i2c_2_peripheral_init(void) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  i2c__initialize(I2C__2, i2c_speed_hz, clock__get_peripheral_clock_hz());
}

static void i2c_1_pin_init(void) {}

static void i2c_2_pin_init(void) {}