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
#include "lcd_driver.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"

typedef char songname_t[16]; // not quite sure what the purpose of this is, im pretty sure its used in app_cli.c
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void screen_control_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
size_t song_number;
int volume;

// Change port and pin for buttons
gpio_s up_button = {1, 23};  // port,pin
gpio_s down_button = {2, 1}; // port, pin

static void initialize_buttons() {
  gpio__construct_as_input(1, 23); // port, pin
  gpio__construct_as_input(2, 1);  // port, pin
}
// flash: python nxp-programmer/flash.py

void main(void) {
  sj2_cli__init();
  mp3_decoder__initialize();
  lcd__initialize();
  initialize_buttons();
  song_list__populate();
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(screen_control_task, "screen-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, 512);

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
      fprintf(stderr, "file %d\n", file);
      if (FR_OK == file) {
        while (!f_eof(&songFile)) {
          f_read(&songFile, bytes_512, 512, &byte_reader);
          fprintf(stderr, "reading file \n");
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
      while (!mp3_decoder__needs_data()) { // need to make this
        fprintf(stderr, "%x", bytes_512[i]);
      }
      fprintf(stderr, "Sending to decoder:\n");
      spi_send_to_mp3_decoder(bytes_512[i]); // need to make this
      fprintf(stderr, "sent\n");
    }
  }
} // josh added, double check its where you want

bool check_up_button() {
  if (gpio__get(up_button)) {
    while (gpio__get(up_button)) {
      vTaskDelay(10);
    }
    return true;
  }
  return false;
}

bool check_down_button() {
  if (gpio__get(down_button)) {
    while (gpio__get(down_button)) {
      vTaskDelay(10);
    }
    return true;
  }
  return false;
}

void screen_control_task(void *p) {
  song_number = 0;

  while (1) {
    if (check_up_button && song_number + 1 != song_list__get_item_count()) {
      song_number++;
    } else if (check_down_button && song_number - 1 >= 0) {
      song_number--;
    }

    print_song_list(song_number, volume);
    vTaskDelay(10);
  }
}
