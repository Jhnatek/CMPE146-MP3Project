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
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include <stdbool.h>
#include <stdint.h>
#define SCI_VOL 0x0B
#define volume_up gpio__construct_as_input(1, 10)
#define volume_down gpio__construct_as_input(1, 14)
#define play_pause gpio__construct_as_input(1, 19)

xTaskHandle Player;
// LPC_IOCON->P0_8 &= ~(3 << 3);
// LPC_IOCON->P0_8 |= (1 << 3);
typedef char songname_t[16]; // not quite sure what the purpose of this is, im prettu sure its used in app_cli.c
typedef char songbyte_t[512];
void Play_Pause_Button(void *p);
void Volume_Control(void *p);
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
volumeControl(bool higher, bool init);
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

uint8_t volume_level = 5;

// flash: python nxp-programmer/flash.py

void main(void) {
  sj2_cli__init();
  mp3_decoder__initialize();
  xTaskCreate(Play_Pause_Button, "Play/Pause", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(Volume_Control, "Volume Control", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, &Player);
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
      // fprintf(stderr, "%x", bytes_512[i]); used for testing milestone 
    uint8_t alternate_status = 1;
      while (!mp3_decoder__needs_data()) { // need to make this
        fprintf(stderr, "%x", bytes_512[i]);
      }
      // fprintf(stderr, "Sending to decoder:\n");
      spi_send_to_mp3_decoder(bytes_512[i]); // need to make this
      // fprintf(stderr, "sent\n");
      
    }
  
  }
} // josh added, double check its where you want

void Play_Pause_Button(void *p) {
  // gpio1__set_as_input(9);
  bool pause = false;
  bool previous = false;
  while (1) {
    if (gpio__get(play_pause) && !previous) {
      pause = true;
    }
    if (gpio__get(play_pause) && previous) {
      pause = false;
    }
    if (!gpio__get(play_pause)){
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

void Volume_Control(void *p) {
  // Volume up pin: 10 port 1
  // Volume down pin: 14 port 1
  bool increase = false;
  bool decrease = false;
  while (1) {
    if (gpio__get(volume_up)) {
      while (gpio__get(volume_up)){      }
      increase = true;
    } 
    if (gpio__get(volume_down)) {
      while (gpio__get(volume_down)){      }
      decrease = true;
    }

    if (increase) {
      volumeControl(true, false);
      left_vol = false;
    } else if (decrease) {
      volumeControl(false, false);
      right_vol = false;
    }
  }
}
volumeControl(bool higher, bool init) {
  if (higher && volume_level < 8 && !init) {
    volume_level++;
  } else if (!higher && volume_level > 1 && !init) {
    volume_level--;
  }

  if (volume_level == 8) {
    MP3_decoder__sci_write(SCI_VOL, 0x1010);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 7) {
    MP3_decoder__sci_write(SCI_VOL, 0x2020);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 6) {
    MP3_decoder__sci_write(SCI_VOL, 0x2525);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 5) {
    MP3_decoder__sci_write(SCI_VOL, 0x3030);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 4) {
    MP3_decoder__sci_write(SCI_VOL, 0x3535);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 3) {
    MP3_decoder__sci_write(SCI_VOL, 0x4040);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 2) {
    MP3_decoder__sci_write(SCI_VOL, 0x4545);
    fprintf("volume: %i", volume_level);
  } else if (volume_level == 1) {
    MP3_decoder__sci_write(SCI_VOL, 0xFEFE);
    fprintf("volume: %i", volume_level);
  }
}
