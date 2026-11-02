/*
 * 00_Hello - the smallest possible sketch. Run this first.
 *
 * If "Hello" appears, the library and the board are fine. If it does not,
 * run 01_PinCheck, which says what is wrong.
 *
 * On e-paper nothing appears until flush(). The screen flashes black and
 * white for about four seconds while it draws - that is normal.
 */
#include <LonelyBinaryEPaper.h>

// ---- GPIO wiring. Every pin this sketch uses is here - change the numbers to match
// ---- your board; nothing below refers to a GPIO by number. -1 = not connected.
#define PIN_CS     5
#define PIN_DC    17
#define PIN_RST   16
#define PIN_BUSY   4   // e-paper BUSY output
#define PIN_SCLK  18
#define PIN_MOSI  23

LB_EPaper display(LB_LILYGO_T5_213);

void setup() {
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.drawString("Hello,", 10, 10);
  display.drawString("Lonely Binary E-Paper", 10, 30);
  display.flush();
}

void loop() {}
