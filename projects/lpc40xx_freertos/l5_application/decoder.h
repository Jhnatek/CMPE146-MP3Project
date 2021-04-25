# pragma once 

void mp3_decoder__initialize(void);

bool mp3_decoder__needs_data(void);

void spi_send_to_mp3_decoder(char * data);

