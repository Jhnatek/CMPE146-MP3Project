#include "LPC40xx.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp2.h" //SPI driver
#include <stdio.h>
#pragma once

void mp3_decoder__initialize(void);

bool mp3_decoder__needs_data(void);

void spi_send_to_mp3_decoder(char *data);

void MP3_decoder__sci_write(uint8_t address, uint16_t data);
