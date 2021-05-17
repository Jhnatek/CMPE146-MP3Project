#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "LPC40xx.h"
#include "app_cli.h"
#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "ff.h"
#include "gpio.h"
#include "interrupt.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include <stdbool.h>
#include <stdint.h>
#define VOLUME 0x0B
#define BASS 0x02

#define play_pause gpio__construct_as_input(2, 2)
#define volumedecrease gpio__construct_as_input(2, 8)
#define volumeincrease gpio__construct_as_input(0, 17)
#define bassincrease gpio__construct_as_input(2, 6)
#define bassdecrease gpio__construct_as_input(2, 5)
#define trebleincrease gpio__construct_as_input(2, 7)
#define trebledecrease gpio__construct_as_input(2, 9)
// save 2.8 and 0.17 for volume up and down
// the other ports you can use: (note that there are only 8 swtiches, i would evenutlaly like a funciton that will
// switch between bass, treble, and volume, and then we can adjust those using volume+/-) 2.2, 2.5, 2.7, 2.9 2.4, 2.6,
// !2.8, !0.17
void pull_down_switches(void);

xTaskHandle Player;
typedef char songname_t[16]; // not quite sure what the purpose of this is, im pretty sure naveen worked on this
typedef char songbyte_t[512];
void Play_Pause_Button(void *p);
// void volumedecrease_isr(void);
// void volumeincrease_isr(void);
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void volumeControl(bool higher, bool init);
void volume_task(void *p);
// void volumedecrease_task(void *p);
// void bassdecrease_isr(void);
// void bassincrease_isr(void);
// void trebledecrease_isr(void);
// void trebleincrease_isr(void);
void bass_function(bool higher, bool init);
void treble_function(bool higher, bool init);
void write_to_decoder_function(void);
void bass_task(void *p);
void treble_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
SemaphoreHandle_t Decoder_Mutex;
// SemaphoreHandle_t volumeincrease_semaphore;
// SemaphoreHandle_t volumedecrease_semaphore;
// // SemaphoreHandle_t bassdecrease_semaphore;
// SemaphoreHandle_t bassincrease_semaphore;
// SemaphoreHandle_t trebledecrease_semaphore;
// SemaphoreHandle_t trebleincrease_semaphore;

uint8_t volume_level = 5;
uint8_t bass_level = 0;
uint8_t treble_level = 0;
// MP3_decoder__sci_write(VOLUME, 0x3030)

// flash: python nxp-programmer/flash.py

void main(void) {
  pull_down_switches();
  // volumedecrease_semaphore = xSemaphoreCreateBinary();
  // volumeincrease_semaphore = xSemaphoreCreateBinary();
  // bassincrease_semaphore = xSemaphoreCreateBinary();
  // bassdecrease_semaphor+e = xSemaphoreCreateBinary();
  // trebleincrease_semaphore = xSemaphoreCreateBinary();
  // trebledecrease_semaphore = xSemaphoreCreateBinary();
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher, NULL);
  // gpio__attach_interrupt(2, 8, GPIO_INTR__FALLING_EDGE, volumeincrease_isr);
  // gpio__attach_interrupt(0, 17, GPIO_INTR__FALLING_EDGE, volumedecrease_isr);
  // gpio__attach_interrupt(2, 2, GPIO_INTR__FALLING_EDGE, bassincrease_isr);
  // gpio__attach_interrupt(2, 5, GPIO_INTR__FALLING_EDGE, bassdecrease_isr);
  // gpio__attach_interrupt(2, 7, GPIO_INTR__FALLING_EDGE, trebleincrease_isr);
  // gpio__attach_interrupt(2, 9, GPIO_INTR__FALLING_EDGE, trebledecrease_isr);
  // gpio__attach_interrupt(0, 30, GPIO_INTR__FALLING_EDGE, bassincrease_isr);
  // gpio__attach_interrupt(0, 29, GPIO_INTR__FALLING_EDGE, bassdecrease_isr);
  // gpio__attach_interrupt(2, 7, GPIO_INTR__FALLING_EDGE, trebleincrease_isr);
  // gpio__attach_interrupt(2, 9, GPIO_INTR__FALLING_EDGE, trebledecrease_isr);
  // LPC_GPIO0->DIR &= ~(1 << 30);
  // NVIC_EnableIRQ(GPIO_IRQn);
  sj2_cli__init();
  mp3_decoder__initialize();
  xTaskCreate(bass_task, "bass increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(treble_task, "treble increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(Play_Pause_Button, "Play/Pause", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(Volume_Control, "Volume Control", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volume_task, "volumeincrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(volumedecrease_task, "volumedecrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, &Player);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Decoder_Mutex = xSemaphoreCreateMutex();
  Q_songdata = xQueueCreate(1, 512);
  // gpio__attach_interrupt(0, 29, GPIO_INTR__FALLING_EDGE, volumeincrease_isr);

  vTaskStartScheduler();
}

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  songname_t name;
  char bytes_512[512];
  UINT *byte_reader;
  FRESULT file;
  const char *song_pointer = name; // maybe delete
  FIL songFile;
  while (true) {
    if (xQueueReceive(Q_songname, &name, portMAX_DELAY)) {
      file = f_open(&songFile, name, FA_READ);
      if (FR_OK == file) {
        while (!f_eof(&songFile)) {
          f_read(&songFile, bytes_512, 512, &byte_reader);
          xQueueSend(Q_songdata, &bytes_512, portMAX_DELAY);
        }
        f_close(&songFile);
      } else {
        fprintf(stderr, "Failed to open file \n");
      }
    }
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {

    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      // fprintf(stderr, "%x", bytes_512[i]); used for testing milestone
      uint8_t alternate_status = 1;
      while (!mp3_decoder__needs_data()) { // need to make this
      }
      if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
        // fprintf(stderr, "Sending to decoder:\n");
        spi_send_to_mp3_decoder(bytes_512[i]); // need to make this
        // fprintf(stderr, "sent\n");
        xSemaphoreGive(Decoder_Mutex);
      }
    }
  }
} // josh added, double check its where you want

void Play_Pause_Button(void *p) {
  bool pause = false;
  bool previous = false;
  while (1) {
    vTaskDelay(10);
    if (gpio__get(play_pause) && !previous) {
      pause = true;
    }
    if (gpio__get(play_pause) && previous) {
      pause = false;
    }
    if (!gpio__get(play_pause)) {
      previous = pause;
    }
    if (pause) {
      vTaskSuspend(Player);
      fprintf(stderr, "MUSIC PAUSED!");
    }
    if (!pause) {
      vTaskResume(Player);
    }
  }
}

// void Volume_Control(void *p) {
//   // Volume up pin: 10 port 1
//   // Volume down pin: 14 port 1
//   bool increase = false;
//   bool decrease = false;
//   while (1) {
//     vTaskDelay(10);
//     if (gpio__get(volume_up)) {
//       while (gpio__get(volume_up)) {
//       }
//       increase = true;
//     } else if (gpio__get(volume_down)) {
//       while (gpio__get(volume_down)) {
//       }
//       decrease = true;
//     } else {
//       if (!gpio__get(volume_up)) {
//         increase = false;
//       } else if (!gpio__get(volume_down)) {
//         decrease = false;
//       }
//     }

//     if (increase) {
//       volumeControl(true, false);
//       increase = false;
//     } else if (decrease) {
//       volumeControl(false, false);
//       decrease = false;
//     }
//   }
// }

void volumeControl(bool higher, bool init) {

  fprintf(stderr, "GOT INTO VOLUMECONTROL\n");
  if (higher && volume_level < 8 && !init) {
    volume_level++;
  } else if (!higher && volume_level > 1 && !init) {
    volume_level--;
  }
  fprintf(stderr, "GOT PAST IF CONDITIONS\n");
  if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
    switch (volume_level) {
    case 8:
      MP3_decoder__sci_write(VOLUME, 0x1010);
      // fprintf("volume: %i", volume_level);
      break;
    case 7:
      MP3_decoder__sci_write(VOLUME, 0x2020);
      // fprintf("volume: %i", volume_level);
      break;
    case 6:
      MP3_decoder__sci_write(VOLUME, 0x2525);
      // fprintf("volume: %i", volume_level);
      break;
    case 5:
      MP3_decoder__sci_write(VOLUME, 0x3030);
      // fprintf("volume: %i", volume_level);
      break;
    case 4:
      MP3_decoder__sci_write(VOLUME, 0x3535);
      // fprintf("volume: %i", volume_level);
      break;
    case 3:
      MP3_decoder__sci_write(VOLUME, 0x3030);
      // fprintf("volume: %i", volume_level);
      break;
    case 2:
      MP3_decoder__sci_write(VOLUME, 0x4545);
      // fprintf("volume: %i", volume_level);
      break;
    case 1:
      MP3_decoder__sci_write(VOLUME, 0xFEFE);
      // fprintf("volume: %i", volume_level);
      break;
    default:
      MP3_decoder__sci_write(VOLUME, 0x3535);
      // fprintf("volume: %i", volume_level);
    }
    fprintf(stderr, "GOT PAST SWITCHES\n");
    xSemaphoreGive(Decoder_Mutex);
  }
}

// void volumeincrease_isr(void) { xSemaphoreGiveFromISR(volumeincrease_semaphore, NULL); }
// void volumedecrease_isr(void) { xSemaphoreGiveFromISR(volumedecrease_semaphore, NULL); }

void volume_task(void *p) {
  while (1) {
    vTaskDelay(10);
    if (xSemaphoreTake(volumeincrease_semaphore, portMAX_DELAY)) {
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      volumeControl(true, false);
      vTaskDelay(10);
    }
  }
  if (xSemaphoreTake(volumedecrease_semaphore, portMAX_DELAY)) {
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      volumeControl(false, false);
      // break;
      vTaskDelay(10);
    }
  xSemaphoreGive(volumeincrease_semaphore);
  vTaskDelay(10);
}

// void volumedecrease_task(void *p) {
//   while (true) {
//     vTaskDelay(10);
//     if (xSemaphoreTake(volumedecrease_semaphore, portMAX_DELAY)) {
//       vTaskDelay(10);
//       fprintf(stderr, "interrupt detected");
//       volumeControl(false, false);
//       // break;
//       vTaskDelay(10);
//     }
//   }
//   xSemaphoreGive(volumedecrease_semaphore);
//   vTaskDelay(250);
// }
// void gpio_interrupt(void) {
//   fprintf(stderr, "Interrupt has been received!!"); // prints that interrupt has been detected
//   gpio__interrupt_dispatcher();                     // locates interrupt pin
//   xSemaphoreGiveFromISR(volumedecrease_semaphore, NULL);
//   volumedecrease_task;
//   LPC_GPIOINT->IO0IntClr |= (1 << 30);
// }

void pull_down_switches(void) {
  gpio__enable_pull_down_resistors(play_pause); // Josh needs this because the buttons are active high
}

void bass_function(bool higher, bool init) {
  fprintf(stderr, "GOT INTO BASS FUNCTION\n");
  if (higher && bass_level < 5 && !init) {
    bass_level++;
  } else if (!higher && bass_level > 1 && !init) {
    bass_level--;
  }
  write_to_decoder_function();
}

void treble_function(bool higher, bool init) {
  fprintf(stderr, "GOT INTO TREBLE FUNCTION\n");
  if (higher && treble_level < 7 && !init) {
    treble_level++;
  } else if (!higher && treble_level > -8 && !init) {
    treble_level--;
  }
  write_to_decoder_function();
}

void write_to_decoder_function(void) {
  if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
    uint16_t bass_treble;
    bass_treble = (((treble_level << 12) & 0xF000) + 0x0F00 + (((bass_level * 3) << 4) & 0x00F0) + 0x000F);
    MP3_decoder__sci_write(BASS, bass_treble);
    xSemaphoreGive(Decoder_Mutex);
  }
}

// void bassdecrease_isr(void) { xSemaphoreGiveFromISR(bassdecrease_semaphore, NULL); }
// void bassincrease_isr(void) { xSemaphoreGiveFromISR(bassincrease_semaphore, NULL); }
// void trebledecrease_isr(void) { xSemaphoreGiveFromISR(trebledecrease_semaphore, NULL); }
// void trebleincrease_isr(void) { xSemaphoreGiveFromISR(trebleincrease_semaphore, NULL); }

void bass_task(void *p) {
  while (true) {
    vTaskDelay(10);
    if (gpio__get(bassdecrease)) {
      while (gpio__get(bassdecrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      bass_function(false, false);
      // break;
      vTaskDelay(10);
    }

    if (gpio__get(bassincrease)) {
      while (gpio__get(bassincrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      bass_function(true, false);
      // break;
      vTaskDelay(10);
    }
    vTaskDelay(10);
  }
}

void treble_task(void *p) {
  while (true) {
    vTaskDelay(10);
    if (gpio__get(trebledecrease)) {
      while (gpio__get(trebledecrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      treble_function(false, false);
      // break;
      vTaskDelay(10);
    }
    if (gpio__get(trebleincrease)) {
      while (gpio__get(trebleincrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      treble_function(true, false);
      // break;
      vTaskDelay(10);
    }
    vTaskDelay(10);
  }
}