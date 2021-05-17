#include "FreeRTOS.h"
#include "LED_control.h"
#include "LPC40xx.h"
#include "app_cli.h"
#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "ff.h"
#include "gpio.h"
#include "interrupt.h"
#include "lcd_driver.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "song_list.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define VOLUME 0x0B
#define BASS 0x02

// button definitions//
////////////////////////////////////////////////////
#define play_pause gpio__construct_as_input(0, 1) // normally 0,1
#define volume_up gpio__construct_as_input(2, 8)
#define volume_down gpio__construct_as_input(2, 7)
#define forward_button gpio__construct_as_input(2, 6)
#define rewind_button gpio__construct_as_input(2, 5)
#define alternate_button gpio__construct_as_input(2, 4)
////////////////////////////////////////////////////

// various global variables and declarations//
/////////////////////////////////////////////////
bool pause;
bool stay_in_loop;
size_t current_song;
size_t number_of_songs;
uint8_t bass_level = 0;
uint8_t treble_level = 0;
typedef char songname_t[20]; // changed from 16 to 20
typedef char songbyte_t[512];
/////////////////////////////////////////////////

// task declarations
////////////////////////////////////////////////
void Play_Pause_Button(void *p);
void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void volumeincrease_task(void *p);
void volumedecrease_task(void *p);
void screen_control_task(void *p);
void alternate_screen_task(void *p);
void bass_task(void *p);
void treble_task(void *p);
QueueHandle_t Q_songdata;
SemaphoreHandle_t Decoder_Mutex;
SemaphoreHandle_t State_Mutex;
SemaphoreHandle_t pause_mutex;
xTaskHandle Player;
////////////////////////////////////////////////

// volume declaration
///////////////////////////////////////////
void volumeControl(bool higher, bool init);
uint8_t volume_level = 5;
////////////////////////////////////////////

// Pull down declaration
///////////////////////////////
void pull_down_switches(void);
///////////////////////////////

// Menu States
/////////////////////
typedef enum { MENU1, MENU2, PAUSE_VOL, PAUSE_TREB, PAUSE_BASS } menu_state;
menu_state current_state;
songname_t *PAUSED = "       PAUSED       ";
////////////////////

// Set menu function
////////////////////////
void update_menu(void);
////////////////////////

// flash: python nxp-programmer/flash.py

void main(void) {
  pull_down_switches();
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio__interrupt_dispatcher, NULL);
  NVIC_EnableIRQ(GPIO_IRQn);
  sj2_cli__init();
  mp3_decoder__initialize();
  MP3_song__init();
  initialize_pwm();
  test_color();
  MP3_decoder__sci_write(VOLUME, 0x3030); // sets volume initially, otherwise starts at max
  number_of_songs = song_list__get_item_count();
  current_song = 0;
  stay_in_loop = true;
  xTaskCreate(Play_Pause_Button, "Play/Pause", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumeincrease_task, "volumeincrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(volumedecrease_task, "volumedecrease", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_reader_task, "read-task", (4096 / sizeof(void *)), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "play-task", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, &Player);
  xTaskCreate(screen_control_task, "Screen-Controller", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(alternate_screen_task, "alternate-screen", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(bass_task, "bass increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(treble_task, "treble increase", (4096 / sizeof(void *)), NULL, PRIORITY_MEDIUM, NULL);
  Decoder_Mutex = xSemaphoreCreateMutex();
  State_Mutex = xSemaphoreCreateMutex();
  pause_mutex = xSemaphoreCreateMutex();
  Q_songdata = xQueueCreate(1, 512);

  vTaskStartScheduler();
}

void screen_control_task(void *p) {
  bool changed = false;

  while (1) {
    if (gpio__get(forward_button) && current_song + 1 != song_list__get_item_count()) {
      current_song++;
      changed = true;
    } else if (gpio__get(rewind_button) && (current_song != 0)) {
      current_song--;
      changed = true;
    }

    if (changed) {
      stay_in_loop = false;
      // set new menu? probs not
    }
    vTaskDelay(100);
    changed = false;
  }
}

void alternate_screen_task(void *p) {
  while (1) {
    if (gpio__get(alternate_button) && xSemaphoreTake(State_Mutex, 0)) {
      while (gpio__get(alternate_button)) {
      }
      switch (current_state) {
      case MENU2:
        current_state = MENU1;
        break;
      case MENU1:
        current_state = MENU2;
        break;
      case PAUSE_VOL:
        current_state = PAUSE_TREB;
        break;
      case PAUSE_TREB:
        current_state = PAUSE_BASS;
        break;
      case PAUSE_BASS:
        current_state = PAUSE_VOL;
        break;
      }
      xSemaphoreGive(State_Mutex);
      update_menu();
    }
    vTaskDelay(100);
  }
}

void mp3_reader_task(void *p) {
  songname_t *name;
  char bytes_512[512];
  UINT *byte_reader;
  FRESULT file;
  lcd__initialize();
  FIL songFile;
  println_to_screen("       WELCOME      ");
  println_to_screen("                    ");
  println_to_screen(" Press Play to Start");
  while (true) {
    name = (song_list__get_name_for_item_only(current_song));
    // println_to_screen(name); // need to replace with funciton
    file = f_open(&songFile, name, FA_READ);
    if (FR_OK == file) {
      while ((!f_eof(&songFile)) && stay_in_loop) {
        f_read(&songFile, bytes_512, 512, &byte_reader);
        xQueueSend(Q_songdata, &bytes_512, portMAX_DELAY);
      }
      f_close(&songFile);
      if (stay_in_loop) {
        ++current_song;
        current_state = MENU1;
        update_menu();
      } else {
        stay_in_loop = true;
        current_state = MENU1;
        update_menu();
      }
    } else {
      fprintf(stderr, "Failed to open file \n");
    }
  }
}

void mp3_player_task(void *p) {
  char bytes_512[512];

  while (1) {

    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      uint8_t alternate_status = 1;
      while (!mp3_decoder__needs_data()) {
      }
      if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
        spi_send_to_mp3_decoder(bytes_512[i]);
        xSemaphoreGive(Decoder_Mutex);
      }
    }
  }
}

void write_to_decoder_function(void) {
  if (xSemaphoreTake(Decoder_Mutex, portMAX_DELAY)) {
    uint16_t bass_treble;
    bass_treble = (((treble_level << 12) & 0xF000) + 0x0F00 + (((bass_level * 3) << 4) & 0x00F0) + 0x000F);
    MP3_decoder__sci_write(BASS, bass_treble);
    xSemaphoreGive(Decoder_Mutex);
  }
}

void bass_function(bool higher) {
  if (higher && bass_level < 5) {

    bass_level++;
  } else if (!higher && bass_level > 1) {
    bass_level--;
  }
  write_to_decoder_function();
}

void treble_function(bool higher) {
  if (higher && treble_level < 7) {
    treble_level++;
  } else if (!higher && treble_level > -8) {
    treble_level--;
  }
  write_to_decoder_function();
}

void bass_task(void *p) {
  while (true) {
    if (gpio__get(volume_down) && current_state == PAUSE_BASS) {
      while (gpio__get(volume_down)) {
      }
      bass_function(false);
    } else if (gpio__get(volume_up) && current_state == PAUSE_BASS) {
      while (gpio__get(volume_up)) {
      }
      bass_function(true);
    }
    vTaskDelay(100);
  }
}

void treble_task(void *p) {
  while (true) {
    if (gpio__get(volume_down) && current_state == PAUSE_TREB) {
      while (gpio__get(volume_down)) {
      }
      treble_function(false);
    }
    if (gpio__get(volume_up) && current_state == PAUSE_TREB) {
      while (gpio__get(volume_up)) {
      }
      treble_function(true);
    }
    vTaskDelay(100);
  }
}

void Play_Pause_Button(void *p) {
  pause = true; // changed to true so that it starts paused
  bool previous = false;
  while (1) {
    if (gpio__get(play_pause) && !previous) {
      pause = true;
      current_state = PAUSE_VOL;
      update_menu();
    }
    if (gpio__get(play_pause) && previous) {
      pause = false;
      current_state = MENU1;
      update_menu();
    }
    if (!gpio__get(play_pause)) {
      previous = pause;
    }
    if (pause) {
      vTaskSuspend(Player);
    }
    if (!pause) {
      vTaskResume(Player);
    }
    vTaskDelay(100);
  }
}

void volumeControl(bool higher, bool init) {

  if (higher && volume_level < 16) {
    volume_level++;
  } else if (!higher && volume_level > 0) {
    volume_level--;
  }
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
      break;
    default:
      MP3_decoder__sci_write(VOLUME, 0x5050);
    }
    update_menu();
    xSemaphoreGive(Decoder_Mutex);
  }
}

void volumeincrease_task(void *p) {
  while (1) {
    if (gpio__get(volume_up) && current_state != PAUSE_TREB && current_state != PAUSE_BASS) {
      volumeControl(true, false);
    }
    vTaskDelay(100);
  }
}

void volumedecrease_task(void *p) {
  while (true) {
    if (gpio__get(volume_down) && current_state != PAUSE_TREB && current_state != PAUSE_BASS) {
      volumeControl(false, false);
    }
    vTaskDelay(100);
  }
}

void pull_down_switches(void) {
  gpio__enable_pull_down_resistors(play_pause); // Josh needs this because the buttons are active high
  gpio__enable_pull_down_resistors(volume_up);
  gpio__enable_pull_down_resistors(volume_down);
  gpio__enable_pull_down_resistors(rewind_button);
  gpio__enable_pull_down_resistors(forward_button);
  gpio__enable_pull_down_resistors(alternate_button);
}

void update_menu(void) {
  char buffer[20];
  char *buffer_pointer1;
  char *buffer_pointer2;
  char *buffer_pointer3;
  lcd_clear();
  if (xSemaphoreTake(State_Mutex, 0)) {
    switch (current_state) {
    case MENU1:
      buffer_pointer1 = song_list__get_name_for_item(1, current_song);
      center_text_to_screen(buffer_pointer1);
      buffer_pointer2 = song_list__get_name_for_item(2, current_song);
      center_text_to_screen(buffer_pointer2);
      buffer_pointer3 = song_list__get_name_for_item(3, current_song);
      center_text_to_screen(buffer_pointer3);
      sprintf(buffer, "      Vol = %d       ", volume_level);
      println_to_screen(buffer);
      break;
    case MENU2:
      buffer_pointer1 = song_list__get_name_for_item(4, current_song);
      center_text_to_screen(buffer_pointer1);
      buffer_pointer2 = song_list__get_name_for_item(5, current_song);
      center_text_to_screen(buffer_pointer2);
      println_to_screen("                    ");
      sprintf(buffer, " Vol = %d       ", volume_level);
      println_to_screen(buffer);
      break;
    case PAUSE_VOL:
      println_to_screen("                    ");
      println_to_screen(PAUSED);
      println_to_screen("                    ");
      sprintf(buffer, "      Vol = %d       ", volume_level);
      println_to_screen(buffer);
      break;
    case PAUSE_TREB:
      println_to_screen("                    ");
      println_to_screen(PAUSED);
      println_to_screen("                    ");
      sprintf(buffer, "     Treb = %d       ", treble_level);
      println_to_screen(buffer);
      break;
    case PAUSE_BASS:
      println_to_screen("                    ");
      println_to_screen(PAUSED);
      println_to_screen("                    ");
      sprintf(buffer, "     Bass = %d       ", treble_level);
      println_to_screen(buffer);
      break;
    }
    xSemaphoreGive(State_Mutex);
  }
}
