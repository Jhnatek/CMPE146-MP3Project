/******************************************************************************
red-plus-buttons.ino
Byron Jacquot @ SparkFun Electronics
1/6/2015

Example to drive the red LEDs and scan the buttons of the RGB button pad.

Exercise 2 in a series of 3.
https://learn.sparkfun.com/tutorials/button-pad-hookup-guide/exercise-2-monochrome-plus-buttons

Development environment specifics:
Developed in Arduino 1.6.5
For an Arduino Mega 2560

This code is released under the [MIT License](http://opensource.org/licenses/MIT).

Distributed as-is; no warranty is given.
******************************************************************************/
// config variables
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#define NUM_LED_COLUMNS (1)
#define NUM_LED_ROWS (4)
#define NUM_BTN_COLUMNS (1)
#define NUM_BTN_ROWS (4)
#define NUM_COLORS (1)
#define MAX_DEBOUNCE (3)
#define btn_column_1 gpio__construct(2, 1)
#define btn_column_2 gpio__construct(2, 4)
#define btn_column_3 gpio__construct(2, 6)
#define btn_column_4 gpio__construct(2, 8)
#define btn_row_1 gpio__construct(0, 17)
#define LED_column_1 gpio__construct(2, 0)
#define LED_column_2 gpio__construct(2, 2)
#define LED_column_3 gpio__construct(2, 5)
#define LED_column_4 gpio__construct(2, 7)
#define LED_row_1 gpio__construct(2, 9)
// 2.0,2.2,2.5,2.7,2.9
// 2.1,2.4,2.6,2.8,0.17
// Global variables
static bool LED_buffer[NUM_LED_COLUMNS][NUM_LED_ROWS];

static const gpio_s btncolumnpins[NUM_BTN_COLUMNS] = {btn_column_1, btn_column_2, btn_column_3, btn_column_4};
static const gpio_s btnrowpins[NUM_BTN_ROWS] = {btn_row_1};
static const gpio_s ledcolumnpins[NUM_LED_COLUMNS] = {LED_column_1, LED_column_2, LED_column_3, LED_column_4};
static const gpio_s colorpins[NUM_LED_ROWS] = {LED_row_1};
static int8_t debounce_count[NUM_BTN_COLUMNS][NUM_BTN_ROWS];

static void setuppins() {
  uint8_t i;

  // initialize
  // select lines
  // LED columns
  for (i = 0; i < NUM_LED_COLUMNS; i++) {
    // arduino:
    // pinMode(ledcolumnpins[i], OUTPUT);
    // sj2:
    gpio__set_as_output(ledcolumnpins[i]);
    // with nothing selected by default
    // arduino:
    // digitalWrite(ledcolumnpins[i], HIGH);
    // sj2:
    gpio__set(ledcolumnpins[i]);
  }

  // button columns
  for (i = 0; i < NUM_BTN_COLUMNS; i++) {
    gpio__set_as_output(btncolumnpins[i]);
    gpio__set(btncolumnpins[i]);
  }

  // button row input lines
  for (i = 0; i < NUM_BTN_ROWS; i++) {
    // arduino:
    // pinMode(btnrowpins[i], INPUT_PULLUP); We dont really have pullup... so i guess just an ordinarry input
    // sj2:
    gpio__set_as_input(btnrowpins[i]);
  }

  // LED drive lines
  for (i = 0; i < NUM_LED_ROWS; i++) {
    // arduino:
    // pinMode(colorpins[i], OUTPUT);
    // digitalWrite(colorpins[i], LOW);
    gpio__set_as_output(colorpins[i]);
    gpio__set(colorpins[i]);
  }

  // Initialize the debounce counter array
  for (uint8_t i = 0; i < NUM_BTN_COLUMNS; i++) {
    for (uint8_t j = 0; j < NUM_BTN_ROWS; j++) {
      debounce_count[i][j] = 0;
    }
  }
}

void scan_matrix() {
  static uint8_t current = 0;
  uint8_t val;
  uint8_t i, j;

  // Select current columns arduino:
  // digitalWrite(btncolumnpins[current], LOW);
  // digitalWrite(ledcolumnpins[current], LOW);
  gpio__reset(btncolumnpins[current]);
  gpio__reset(ledcolumnpins[current]);

  // output LED row values
  for (i = 0; i < NUM_LED_ROWS; i++) {
    if (LED_buffer[current][i]) {
      gpio__set(colorpins[i]);
      // digitalWrite(colorpins[i], HIGH);
    }
  }

  // pause a moment
  delay(1);

  // Read the button inputs
  for (j = 0; j < NUM_BTN_ROWS; j++) {
    // arduino:
    // val = digitalRead(btnrowpins[j]);
    val = gpio__get(btnrowpins[j]);

    if (val == 0) // arduino equivalent is LOW
    {
      // active low: val is low when btn is pressed
      if (debounce_count[current][j] < MAX_DEBOUNCE) {
        debounce_count[current][j]++;
        if (debounce_count[current][j] == MAX_DEBOUNCE) {
          fprintf(stderr, "Key Down \n");
          // Serial.println((current * NUM_BTN_ROWS) + j); //not sure what equivalent is in sj2

          // Do whatever you want to with the button press here:
          // toggle the current LED state
          LED_buffer[current][j] = !LED_buffer[current][j];
        }
      }
    } else {
      // otherwise, button is released
      if (debounce_count[current][j] > 0) {
        debounce_count[current][j]--;
        if (debounce_count[current][j] == 0) {
          fprintf(stderr, "Key Up \n");
          // Serial.println((current * NUM_BTN_ROWS) + j); //not sure what equivalent is

          // If you want to do something when a key is released, do it here:
        }
      }
    }
  } // for j = 0 to 3;

  delay(1);
  // arduino
  // digitalWrite(btncolumnpins[current], HIGH);
  // digitalWrite(ledcolumnpins[current], HIGH);
  // sj2
  gpio__set(btncolumnpins[current]);
  gpio__set(ledcolumnpins[current]);

  for (i = 0; i < NUM_LED_ROWS; i++) {
    // sj2:
    gpio_reset(colorpins[i]);
    // digitalWrite(colorpins[i], LOW);
  }

  current++;
  if (current >= NUM_LED_COLUMNS) {
    current = 0;
  }
}

void setup() {
  // put your setup code here, to run once:
  fprintf(stderr, "Starting Setup...\n");

  // setup hardware
  setuppins();

  // init global variables
  for (uint8_t i = 0; i < NUM_LED_COLUMNS; i++) {
    for (uint8_t j = 0; j < NUM_LED_ROWS; j++) {
      LED_buffer[i][j] = 0;
    }
  }

  fprintf(stderr, "Setup Complete.\n");
}
