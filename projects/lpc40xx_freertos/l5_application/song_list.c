#include "song_list.h"
#include "ff.h"
#include <stdbool.h>
#include <string.h>

// static song_memory_t list_of_songs[32];
size_t number_of_songs;
song_data_t list_of_songs[32] = {0};
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
#define CHAR_SEPARATOR '|'

void MP3_song__init(void) {
  FIL songs_file;
  number_of_songs = 0;

  // Open song_list file
  if ((f_open(&songs_file, "song_list.txt", FA_OPEN_EXISTING | FA_READ) == FR_OK)) {
    while (!f_eof(&songs_file)) {

      song_memory_t song_data_string = {0};
      f_gets(song_data_string, sizeof(song_memory_t) - 1, &songs_file); // extract char per line
      printf(song_data_string);

      // Split String
      int i = 0;
      for (int j = 0; j < number_of_data; j++) {
        int index = 0;
        while (!(song_data_string[i] == '\n' || song_data_string[i] == '\0' ||
                 song_data_string[i] == CHAR_SEPARATOR)) { // Check if whitespace
          // printf("JIndex: %d, Index: %d, Char %c\n", j, index, song_data_string[i]);
          list_of_songs[number_of_songs][j][index++] = song_data_string[i++];
        }
        i++; // Consume '\0' or ',';
      }
#if DEBUG
      printf("Index %d, Filename %s, Songname %s, Artist %s, Album %s, Year %s, Genre %s\n", number_of_songs,
             list_of_songs[number_of_songs][0], list_of_songs[number_of_songs][1], list_of_songs[number_of_songs][2],
             list_of_songs[number_of_songs][3], list_of_songs[number_of_songs][4], list_of_songs[number_of_songs][5]);
#endif
      number_of_songs++;
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

void center_text_to_screen(char *string) {
  int count = 0;
  int number_of_first_space = (20 - strlen(string)) / 2;
  song_memory_t first_whitespace = {0};
  for (int i = 0; i < number_of_first_space; i++) {
    strcat(first_whitespace, " ");
    count++;
  }
  count = count + strlen(string);
  song_memory_t display = {0};
  strcat(display, first_whitespace); // Place first whitespace in front
  strcat(display, string);           // Concatenate frontwhitespace and string text
                                     // while (count < 19) {
  //  strcat(display, " ");
  // }
  strcat(display, first_whitespace);
  println_to_screen(display);
}

size_t song_list__get_item_count(void) { return number_of_songs; }

song_memory_t *song_list__get_name_for_item_only(size_t item_number) {
  song_memory_t *return_pointer = NULL;

  if (item_number < number_of_songs) {
    return_pointer = list_of_songs[item_number];
  }
  return return_pointer;
}

song_memory_t *song_list__get_name_for_item(size_t item_number, size_t current_song) {
  song_memory_t *return_pointer = NULL;

  if (item_number < number_of_songs) {
    return_pointer = list_of_songs[current_song][item_number];
  }
  return return_pointer;
}

char *return_specific_information(song_data desire) {
  char buffer[20] = "";
  int number_of_first_space;
  char *string;
  char *buffer_pointer = &buffer;
  int track;
  switch (desire) {
  case SONG_TITLE:
    string = list_of_songs[1];
    number_of_first_space = (20 - strlen(string)) / 2;
    track = strlen(string);
    break;
  case SONG_ARTIST:
    string = list_of_songs[2];
    number_of_first_space = (20 - strlen(string)) / 2;
    track = strlen(string);
    break;
  case SONG_ALBUM:
    string = list_of_songs[3];
    number_of_first_space = (20 - strlen(string)) / 2;
    track = strlen(string);
    break;
  case SONG_YEAR:
    string = list_of_songs[4];
    number_of_first_space = (20 - strlen(string)) / 2;
    track = strlen(string);
    break;
  case SONG_GENRE:
    string = list_of_songs[5];
    number_of_first_space = (20 - strlen(string)) / 2;
    track = strlen(string);
    break;
  }
  for (int i = 0; i < number_of_first_space; i++) {
    sprintf(buffer, " ");
    track++;
  }
  // song_memory_t display = {0};
  // strcat(buffer, first_whitespace); // Place first whitespace in front
  sprintf(buffer, "%s", string);
  // strcat(buffer, string);           // Concatenate frontwhitespace and string text
  while (track <= 20) {
    sprintf(buffer, " ");
  }
  return buffer_pointer;
}