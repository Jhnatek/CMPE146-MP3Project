#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "app_cli.h"
#include "LPC40xx.h"
#include "semphr.h"


typedef char songname_t[16]; //not quite sure what the purpose of this is, im prettu sure its used in app_cli.c
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

void main(void) {
  Q_songname = xQueueCreate(1, sizeof(songname));
  Q_songdata = xQueueCreate(1, 512);
}

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p) {
  songname_t name; //im assuming what is going to go into Q_songname, i added "_t" to the end of songname
  char bytes_512[512];
 
  while(1) {
    xQueueReceive(Q_songname, &name[0], portMAX_DELAY);
    printf("Received song to play: %s\n", name);
    
    open_file(); //i think this is what we need to work on, there also should be a chip select
    while (!file.end()) {
      read_from_file(bytes_512);
      xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
    }
    close_file();
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task(void *p) {
  char bytes_512[512];
  
  while (1) {
    xQueueReceive(Q_songdata,  &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      while (!mp3_decoder_needs_data()) {
        vTaskDelay(1);
      }
      
      spi_send_to_mp3_decoder(bytes_512[i]);
    }
  }
}
