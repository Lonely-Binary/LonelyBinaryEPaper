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

LB_EPaper display(LB_LILYGO_T5_213);

void setup() {
  display.begin();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.drawString("Hello,", 10, 10);
  display.drawString("Lonely Binary E-Paper", 10, 30);
  display.flush();
}

void loop() {}
