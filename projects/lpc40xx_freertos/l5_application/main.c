#include "FreeRTOS.h"
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
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/*  DEFINITIONS   */
#define VOLUME 0x0B
#define BASS 0x02
#define play_pause gpio__construct_as_input(2, 2)
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
/*  BUTTONS/SWITCHES   */
void Play_Pause_Button(void *p);
void volumedecrease_isr(void);
void volumeincrease_isr(void);
/*  TASKS   */
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void volumeincrease_task(void *p);
void volumedecrease_task(void *p);
void bass_task(void *p);
void treble_task(void *p);
/*  WRITER/REQUIRED FUNCTIONS   */
void volumeControl(bool higher);
void bass_function(bool higher);
void treble_function(bool higher);
void write_to_decoder_function(void);

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
SemaphoreHandle_t Decoder_Mutex;
SemaphoreHandle_t volumeincrease_semaphore;
SemaphoreHandle_t volumedecrease_semaphore;

/*   DECLARATIONS/INITIALIZERS   */
uint8_t volume_level = 5;
uint8_t bass_level = 0;
uint8_t treble_level = 0;

// flash: python nxp-programmer/flash.py

void main(void) {
  pull_down_switches();
  volumedecrease_semaphore = xSemaphoreCreateBinary();
  volumeincrease_semaphore = xSemaphoreCreateBinary();
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher, NULL);
  gpio__attach_interrupt(2, 8, GPIO_INTR__FALLING_EDGE, volumeincrease_isr);
  gpio__attach_interrupt(0, 17, GPIO_INTR__FALLING_EDGE, volumedecrease_isr);
  NVIC_EnableIRQ(GPIO_IRQn);

  /*   TASKS WITH SCHEDULER    */
  sj2_cli__init();
  mp3_decoder__initialize();
  xTaskCreate(bass_task, "bass increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(treble_task, "treble increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(Play_Pause_Button, "Play/Pause", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumeincrease_task, "volumeincrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumedecrease_task, "volumedecrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, &Player);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Decoder_Mutex = xSemaphoreCreateMutex();
  Q_songdata = xQueueCreate(1, 512);

  vTaskStartScheduler();
}

/*         FUNCTION DEFINITIONS           */

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

void volumeControl(bool higher) {

  fprintf(stderr, "GOT INTO VOLUMECONTROL\n");
  if (higher && volume_level < 16) {
    volume_level++;
  } else if (!higher && volume_level > 0) {
    volume_level--;
  }
  fprintf(stderr, "GOT PAST IF CONDITIONS\n");
  if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
    switch (volume_level) {
    case 16:
      MP3_decoder__sci_write(VOLUME, 0x0000);
    case 15:
      MP3_decoder__sci_write(VOLUME, 0x1010);
      break;
    case 14:
      MP3_decoder__sci_write(VOLUME, 0x2020);
      break;
    case 13:
      MP3_decoder__sci_write(VOLUME, 0x3030);
      break;
    case 12:
      MP3_decoder__sci_write(VOLUME, 0x4040);
      break;
    case 11:
      MP3_decoder__sci_write(VOLUME, 0x5050);
      break;
    case 10:
      MP3_decoder__sci_write(VOLUME, 0x6060);
      break;
    case 9:
      MP3_decoder__sci_write(VOLUME, 0x7070);
      break;
    case 8:
      MP3_decoder__sci_write(VOLUME, 0x8080);
      break;
    case 7:
      MP3_decoder__sci_write(VOLUME, 0x9090);
      break;
    case 6:
      MP3_decoder__sci_write(VOLUME, 0xA0A0);
      break;
    case 5:
      MP3_decoder__sci_write(VOLUME, 0xB0B0);
      break;
    case 4:
      MP3_decoder__sci_write(VOLUME, 0xC0C0);
      break;
    case 3:
      MP3_decoder__sci_write(VOLUME, 0xD0D0);
      break;
    case 2:
      MP3_decoder__sci_write(VOLUME, 0xE0E0);
      break;
    case 1:
      MP3_decoder__sci_write(VOLUME, 0xF0F0);
      break;
    case 0:
      MP3_decoder__sci_write(VOLUME, 0xFEFE);
      // fprintf("volume: %i", volume_level);
      break;
    default:
      MP3_decoder__sci_write(VOLUME, 0x5050);
      // fprintf("volume: %i", volume_level);
    }
    fprintf(stderr, "GOT PAST SWITCHES\n");
    xSemaphoreGive(Decoder_Mutex);
  }
}

void volumeincrease_isr(void) { xSemaphoreGiveFromISR(volumeincrease_semaphore, NULL); }
void volumedecrease_isr(void) { xSemaphoreGiveFromISR(volumedecrease_semaphore, NULL); }

void volumeincrease_task(void *p) {
  while (1) {
    vTaskDelay(10);
    if (xSemaphoreTake(volumeincrease_semaphore, portMAX_DELAY)) {
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      volumeControl(true);
      vTaskDelay(10);
    }
  }
  xSemaphoreGive(volumeincrease_semaphore);
  vTaskDelay(250);
}

void volumedecrease_task(void *p) {
  while (true) {
    vTaskDelay(10);
    if (xSemaphoreTake(volumedecrease_semaphore, portMAX_DELAY)) {
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      volumeControl(false);
      // break;
      vTaskDelay(10);
    }
  }
  xSemaphoreGive(volumedecrease_semaphore);
  vTaskDelay(250);
}

void pull_down_switches(void) {
  gpio__enable_pull_down_resistors(play_pause); // Josh needs this because the buttons are active high
}

void bass_function(bool higher) {
  fprintf(stderr, "GOT INTO BASS FUNCTION\n");
  if (higher && bass_level < 5) {

    bass_level++;
  } else if (!higher && bass_level > 1) {
    bass_level--;
  }
  write_to_decoder_function();
}

void treble_function(bool higher) {
  fprintf(stderr, "GOT INTO TREBLE FUNCTION\n");
  if (higher && treble_level < 7) {
    treble_level++;
  } else if (!higher && treble_level > -8) {
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

void bass_task(void *p) {
  while (true) {
    vTaskDelay(10);
    if (gpio__get(bassdecrease)) {
      while (gpio__get(bassdecrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      bass_function(false);
      // break;
      vTaskDelay(10);
    }

    if (gpio__get(bassincrease)) {
      while (gpio__get(bassincrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      bass_function(true);
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
      treble_function(false);
      // break;
      vTaskDelay(10);
    }
    if (gpio__get(trebleincrease)) {
      while (gpio__get(trebleincrease)) {
      }
      vTaskDelay(10);
      fprintf(stderr, "interrupt detected");
      treble_function(true);
      // break;
      vTaskDelay(10);
    }
  }
}