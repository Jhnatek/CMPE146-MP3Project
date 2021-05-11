#pragma once
#include "song_list.h"
#include <stdint.h>

extern const uint8_t lcd_rows;
extern const uint8_t lcd_columns;

void lcd__initialize(void);

void println_to_screen(char *str);

void lcd_clear(void);

void setting_command(char byte);

void special_command(char byte);
