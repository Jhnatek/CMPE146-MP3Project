#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "matrix.c"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
// flash: python nxp-programmer/flash.py

int main(void) {
  while () {
    scan_matrix();
  }
  return 0;
}
