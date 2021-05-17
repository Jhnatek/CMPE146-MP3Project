#pragma once
#include <stddef.h> // size_t
#include <stdint.h>
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
typedef char song_memory_t[128];
typedef enum {
  SONG_TITLE,
  SONG_ARTIST,
  SONG_ALBUM,
  SONG_YEAR,
  SONG_GENRE,
} song_data;

/*
 * [0] = filename, [1] = songname, [2] = artist, [3] = album, [4] = genre, [5] = year
 */
#define number_of_data 6
typedef song_memory_t song_data_t[number_of_data];

/* Do not declare variables in a header file */
#if 0
static song_memory_t list_of_songs[32];
static size_t number_of_songs;
#endif

void MP3_song__init(void); // Populating Songs

size_t MP3_song__get_overall_size(void);

song_memory_t *song_list__get_name_for_item(size_t item_number, size_t current_song);

char *return_specific_information(song_data desire);

void center_text_to_screen(char *string);

song_memory_t *song_list__get_name_for_item_only(size_t item_number);