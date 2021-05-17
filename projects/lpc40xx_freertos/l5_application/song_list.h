#pragma once
#include <stddef.h> // size_t
#include <stdint.h>
#include <stdio.h>

typedef char song_memory_t[128];

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
