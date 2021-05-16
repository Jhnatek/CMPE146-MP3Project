#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "LPC40xx.h"
#include "app_cli.h"
#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "ff.h"
#include "lcd_driver.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"

typedef char songname_t[16]; // not quite sure what the purpose of this is, im prettu sure its used in app_cli.c
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

// flash: python nxp-programmer/flash.py

void main(void) {
  sj2_cli__init();
  mp3_decoder__initialize();
  lcd__initialize();
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, 512);

  vTaskStartScheduler();
}

// Reader tasks receives song-name over Q_songname to start reading it
// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  songname_t *name;
  char bytes_512[512];
  UINT *byte_reader;
  FRESULT file;
  char current_song[32];
  lcd__initialize(); // wont work right in main. need to make print statments after reader_task
  FIL songFile;
  while (true) {
    // if (xQueueReceive(Q_songname, &name, portMAX_DELAY)) {
    name = (song_list__get_name_for_item(current_song));
    println_to_screen(name); // eventually get rid of
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
    //}
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      while (!mp3_decoder__needs_data()) { // need to make this
        fprintf(stderr, "%x", bytes_512[i]);
      }
      fprintf(stderr, "Sending to decoder:\n");
      spi_send_to_mp3_decoder(bytes_512[i]); // need to make this
      fprintf(stderr, "sent\n");
    }
  }
} // josh added, double check its where you want
