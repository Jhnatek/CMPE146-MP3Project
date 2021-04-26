#include "decoder.h"
// josh moved includes to .h
static const gpio_s SCLK = {0, 1}; // slave clock // pin 1.0 //Josh changed this to pin 0.1i
static const gpio_s SI = {1, 1};   // Slave in //pin 1.1
static const gpio_s SO = {1, 4};   // slave out  // pin 1.4

static const gpio_s RST = {0, 8};   // data request // pin 0.8 //josh renamed from reset to RST
static const gpio_s CS = {0, 26};   // CS // pin 0.26
static const gpio_s XDCS = {1, 31}; // data cs // pin 1.31
static const gpio_s DREQ = {1, 20}; // data request // pin 1.20

static void MP3_decoder__softreset(void) {
  MP3_decoder__sci_write(0x0, 0x800 | 0x4);
  delay__ms(100);
}

static void MP3_decoder__reset(void) {
  if (!gpio__get(RST)) {
    gpio__reset(RST);
    delay__ms(100);
    gpio__set(RST);
  }
  gpio__set(RST);
  gpio__set(RST);
  delay__ms(100);
  MP3_decoder__softreset();
  delay__ms(100);

  MP3_decoder__sci_write(0x3, 0x6000);
}

void mp3_decoder__initialize(void) {

  // Configure as SPI pins
  gpio__set_function(SI, GPIO__FUNCTION_4);
  gpio__set_function(SO, GPIO__FUNCTION_4);
  gpio__set_function(SCLK, GPIO__FUNCTION_4);

  // Set GPIO Pins input/output
  gpio__set_as_output(RST);
  gpio__set_as_output(CS);
  gpio__set_as_output(XDCS);
  gpio__set_as_input(DREQ);

  ssp2__initialize(1000); // Change frequency
  MP3_decoder__reset();
}

bool mp3_decoder__needs_data(void) {
  // DREQ === BUSY FLAG
  // DREQ is HIGH(1) == Data Ready
  // DREQ is LOW(0) == Busy (Not Ready)
  return gpio__get(DREQ);
}

void spi_send_to_mp3_decoder(char *data) {
  gpio__reset(XDCS);
  ssp2__exchange_byte(data);
  gpio__set(XDCS);
}
