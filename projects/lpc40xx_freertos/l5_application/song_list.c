#include "song_list.h"
#include "ff.h"
#include <stdbool.h>
#include <string.h>

// static song_memory_t list_of_songs[32];
static song_data_t list_of_songs[32] = {0};
static size_t number_of_songs;

/*
static void song_list__handle_filename(const char *filename) {
  // This will not work for cases like "file.mp3.zip"
  if (NULL != strstr(filename, ".mp3")) {
    // printf("Filename: %s\n", filename);

    // Dangerous function: If filename is > 128 chars, then it will copy extra bytes leading to memory corruption
    // strcpy(list_of_songs[number_of_songs], filename);

    // Better: But strncpy() does not guarantee to copy null char if max length encountered
    // So we can manually subtract 1 to reserve as NULL char
    strncpy(list_of_songs[number_of_songs], filename, sizeof(song_memory_t) - 1);

    // Best: Compensates for the null, so if 128 char filename, then it copies 127 chars, AND the NULL char
    // snprintf(list_of_songs[number_of_songs], sizeof(song_memory_t), "%.149s", filename);

    ++number_of_songs;
    // or
    // number_of_songs++;
  }
}*/

/*
 Input file format
    <filename1> <song name1> <artist1> <album1> <genre1> <year1>
    <filename2> <song name2> <artist2> <album2> <genre2> <year2>
    ...
*/

#define DEBUG 1 // Make this 1 to turn on DEBUG

void MP3_song__init(void) {
  FIL songs_file;
  number_of_songs = 0;

  // Open song_list file
  if ((f_open(&songs_file, "song_list.txt", FA_OPEN_EXISTING | FA_READ) == FR_OK)) {
    while (!f_eof(&songs_file)) {

      song_memory_t song_data_string = {0};
      f_gets(song_data_string, sizeof(song_memory_t) - 1, &songs_file); // extract char per line

      // Split String
      for (int i = 0; i < sizeof(song_memory_t) && song_data_string[i] != '\n'; i++) {
        for (int j = 0; j < number_of_data; j++) {
          if (song_data_string[i] == '\0' || song_data_string[i] == ' ') // Check if whitespace
            continue;
          list_of_songs[i][j] = song_data_string[i];
        }
#if DEBUG
        printf("Index %d, Filename %s, Songname %s, Artist %s, Album %s, Genre %s, Year %s\n", i, list_of_songs[i][0],
               list_of_songs[i][1], list_of_songs[i][2], list_of_songs[i][3], list_of_songs[i][4], list_of_songs[i][5]);
#endif
        number_of_songs++;
      }
    }
  }
}
/*
void song_list__populate(void) {
  FRESULT res;
  static FILINFO file_info;
  const char *root_path = "/";

  DIR dir;
  res = f_opendir(&dir, root_path);

  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &file_info); /* Read a directory item
      if (res != FR_OK || file_info.fname[0] == 0) {
        break; /* Break on error or end of dir
      }

      if (file_info.fattrib & AM_DIR) {
        /* Skip nested directories, only focus on MP3 songs at the root
      } else { /* It is a file.
        song_list__handle_filename(file_info.fname);
      }
    }
    f_closedir(&dir);
  }
}*/

size_t song_list__get_item_count(void) { return number_of_songs; }

const char *song_list__get_name_for_item(size_t item_number) {
  const char *return_pointer = NULL;

  if (item_number < number_of_songs) {
    return_pointer = list_of_songs[item_number][0]; //Return song filename
  }
  return return_pointer;
}
