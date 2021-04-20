#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "LPC40xx.h"
#include "app_cli.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"

typedef char songname_t[16]; // not quite sure what the purpose of this is, im prettu sure its used in app_cli.c
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

void main(void) {
  sj2_cli__init();
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, 512);

  vTaskStartScheduler();
}

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  songname_t name;
  char bytes_512[512];
  UINT byte_reader = 0;
  while (true) {
    if (xQueueReceive(Q_songname, &name, portMAX_DELAY)) {
      const char *song_pointer = name;
      FIL songFile;
      FRESULT file = f_open(&songFile, song_pointer, (FA_READ));
      if (FR_OK == file) {
        f_read(&songFile, bytes_512, 512, &byte_reader);
        if (byte_reader != 0) {
          xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
        } else {
          printf("ERROR: FAILED TO PLAY!!\n");
        }
      }
      f_close(&file);
    }
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      // while (!mp3_decoder_needs_data()) { //need to make this
      // vTaskDelay(1);
      fprintf(stderr, "%x", bytes_512[i]);
    }

    // spi_send_to_mp3_decoder(bytes_512[i]); //need to make this
  }
}
