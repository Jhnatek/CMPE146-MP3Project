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
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include <stdbool.h>
#include <stdint.h>
#define VOLUME 0x0B
#define play_pause gpio__construct_as_input(1, 19)

xTaskHandle Player;
// LPC_IOCON->P0_8 &= ~(3 << 3);
// LPC_IOCON->P0_8 |= (1 << 3);
typedef char songname_t[16]; // not quite sure what the purpose of this is, im prettu sure its used in app_cli.c
typedef char songbyte_t[512];
void Play_Pause_Button(void *p);
void gpio_interrupt(void);
// void Volume_Control(void *p);
void volumedecrease_isr(void);
void volumeincrease_isr(void);
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void volumeControl(bool higher, bool init);
void volumeincrease_task(void *p);
void volumedecrease_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
SemaphoreHandle_t Decoder_Mutex;
SemaphoreHandle_t volumeincrease_semaphore;
SemaphoreHandle_t volumedecrease_semaphore;


uint8_t volume_level = 5;
// MP3_decoder__sci_write(VOLUME, 0x3030)

// flash: python nxp-programmer/flash.py

void main(void) {
  
  volumedecrease_semaphore = xSemaphoreCreateBinary();
  gpio__attach_interrupt(0, 30, GPIO_INTR__FALLING_EDGE, volumedecrease_isr);
  LPC_GPIO0->DIR &= ~(1 << 30);
  sj2_cli__init();
  mp3_decoder__initialize();
  xTaskCreate(Play_Pause_Button, "Play/Pause", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(Volume_Control, "Volume Control", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumeincrease_task, "volumeincrease", (1024 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumedecrease_task, "volumedecrease", (1024 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, &Player);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Decoder_Mutex = xSemaphoreCreateMutex();
  volumeincrease_semaphore = xSemaphoreCreateBinary();
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
  if (higher && volume_level < 8 && !init) {
    volume_level++;
  } else if (!higher && volume_level > 1 && !init) {
    volume_level--;
  }
  if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
    vTaskDelay(10);
    if (volume_level == 8) {
      MP3_decoder__sci_write(VOLUME, 0x1010);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 7) {
      MP3_decoder__sci_write(VOLUME, 0x2020);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 6) {
      MP3_decoder__sci_write(VOLUME, 0x2525);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 5) {
      MP3_decoder__sci_write(VOLUME, 0x3030);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 4) {
      MP3_decoder__sci_write(VOLUME, 0x3535);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 3) {
      MP3_decoder__sci_write(VOLUME, 0x4040);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 2) {
      MP3_decoder__sci_write(VOLUME, 0x4545);
      fprintf("volume: %i", volume_level);
    } else if (volume_level == 1) {
      MP3_decoder__sci_write(VOLUME, 0xFEFE);
      fprintf("volume: %i", volume_level);
    }
    vTaskDelay(10);
    xSemaphoreGive(Decoder_Mutex);
  }
}

void volumeincrease_isr(void) { xSemaphoreGiveFromISR(volumeincrease_semaphore, NULL); }
void volumedecrease_isr(void) { xSemaphoreGiveFromISR(volumedecrease_semaphore, NULL); }

void volumeincrease_task(void *p) {
  while (1) {
    if (xSemaphoreTake(volumeincrease_semaphore, portMAX_DELAY)) {
      volumeControl(true, false);
    }
  }
  xSemaphoreGive(volumeincrease_semaphore);
}

void volumedecrease_task(void *p) {
  while (1) {
    if (xSemaphoreTake(volumedecrease_semaphore, portMAX_DELAY)) {
      fprintf("interrupt detected");
      volumeControl(false, false);
    }
  }
  xSemaphoreGive(volumedecrease_semaphore);
}
void gpio_interrupt(void) {
  fprintf(stderr, "Interrupt has been received!!"); // prints that interrupt has been detected
  gpio0__interrupt_dispatcher0();                   // locates interrupt pin
  LPC_GPIOINT->IO0IntClr |= (1 << 30);
}