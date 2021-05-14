#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "matrix.c"
// flash: python nxp-programmer/flash.py

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  // If you have the ESP32 wifi module soldered on the board, you can try uncommenting this code
  // See esp32/README.md for more details
  // uart3_init();                                                                     // Also include:  uart3_init.h
  // xTaskCreate(esp32_tcp_hello_world_task, "uart3", 1000, NULL, PRIORITY_LOW, NULL); // Include esp32_task.h

  puts("Starting RTOS,  why hello there world");
  // LAB 2

  xTaskCreate(task_one, "aaaaaa", 1000, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(task_two, "bbbbbb", 1000, NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

