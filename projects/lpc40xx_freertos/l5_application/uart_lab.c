#include "uart_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"

static const uint32_t Uart2_Power_Bit = (1 << 24);
static const uint32_t Uart3_Power_Bit = (1 << 25);

static LPC_UART_TypeDef *UArts[] = {LPC_UART2, LPC_UART2, LPC_UART2, LPC_UART3}; // index 0-2 for UART2

static QueueHandle_t Uart_Rx_Queue;

void Uart__Enable_Dll_Dlm(Uart_Number_E uart) { UArts[uart]->LCR |= (1 << 7); }

void Uart__Disable_Dll_Dlm(Uart_Number_E Uart) { UArts[Uart]->LCR &= ~(1 << 7); }

void Uart_Driver__Init(Uart_Number_E Uart, uint32_t Peripheral_Clock, uint32_t Baud_Rate) {
  switch (Uart) {
  case UART__2:
    LPC_SC->PCONP |= Uart2_Power_Bit;
    break;
  case UART__3:
    LPC_SC->PCONP |= Uart3_Power_Bit;
    break;
  default:
    // do nothing
    return;
  }
  UArts[Uart]->LCR = (3 << 0); // eight bit char lenghth set

  Uart_Driver__Set_Baud_Rate(Uart, Peripheral_Clock, Baud_Rate);
}

void Uart_Driver__Set_Baud_Rate(Uart_Number_E Uart, uint32_t Peripheral_Clock, uint32_t Baud_Rate) {
  Uart__Enable_Dll_Dlm(Uart);
  {
    const uint32_t Register_Value = (Peripheral_Clock) / (16 * Baud_Rate);
    UArts[Uart]->DLM = (Register_Value >> 8) & 0xFF;
    UArts[Uart]->DLL = Register_Value & 0xFF;
  }
  Uart__Disable_Dll_Dlm(Uart);

  UArts[Uart]->FDR &= ~(7 << 0); // disable fractional divider
}

bool Uart_Driver__Polled_Put(Uart_Number_E Uart, char Output_Byte) {
  while (!(UArts[Uart]->LSR & (1 << 5))) {
    ; // wait until there is something to write
  }
  UArts[Uart]->THR |= Output_Byte;
  return true;
}
