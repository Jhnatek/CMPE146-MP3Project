#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  UART__2 = 2,
  UART__3 = 3,
} Uart_Number_E;

void Uart_Driver__Init(Uart_Number_E Uart, uint32_t Peripheral_Clock, uint32_t Baud_Rate);

bool Uart_Driver__Polled_Put(Uart_Number_E Uart, char output_byte);

void Uart_Driver__Set_Baud_Rate(Uart_Number_E Uart, uint32_t Peripheral_Clock, uint32_t Baud_Rate);
