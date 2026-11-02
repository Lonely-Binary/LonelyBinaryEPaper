/*
 * 04_Rotation - which way is up.
 *
 * A panel has one native orientation, and on e-paper it is usually portrait:
 * the 2.13" here is natively 122 wide and 250 tall. The library starts it the
 * way the board is normally held (landscape for the T5), and setRotation()
 * turns it in steps of 90 degrees:
 *
 *   0  portrait      2  portrait, upside down
 *   1  landscape     3  landscape, upside down
 *
 * !! width() and height() change with it !!
 *   After setRotation(0) the screen is 122 x 250; after setRotation(1) it is
 *   250 x 122. Code that uses width() and height() - never the numbers -
 *   lays itself out correctly either way. That is the only rule here.
 *
 * Rotation on e-paper is done in software, while drawing, and costs nothing
 * you would notice: the refresh takes seconds, the drawing microseconds.
 *
 * Four full refreshes, one per rotation, about ten seconds in all. Each screen
 * shows an arrow pointing to its own top, and its own size.
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

static void screen(uint8_t r) {
  display.setRotation(r);
  const int16_t w = display.width(), h = display.height();
  display.fillScreen(LB_WHITE);
  display.drawRect(0, 0, w, h, LB_BLACK);

  // An arrow pointing at the top edge, centred
  const int16_t cx = w / 2;
  display.fillTriangle(cx, 6, cx - 14, 26, cx + 14, 26, LB_BLACK);
  display.fillRect(cx - 5, 26, 10, 22, LB_BLACK);
  display.setTextColor(LB_BLACK);
  display.drawString("UP", cx - display.textWidth("UP") / 2, 52);

  char buf[24];
  snprintf(buf, sizeof(buf), "rotation %d", r);
  display.drawString(buf, (w - display.textWidth(buf)) / 2, h - 42);
  snprintf(buf, sizeof(buf), "%d x %d", w, h);
  display.drawString(buf, (w - display.textWidth(buf)) / 2, h - 22);

  display.flush();
  Serial.printf("rotation %d: %d x %d, %ld ms\n", r, w, h, display.lastRefreshMs());
}

void setup() {
  Serial.begin(115200);
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  const uint8_t start = display.rotation();   // the panel's own default
  for (uint8_t i = 0; i < 4; i++) {
    screen((start + i) & 3);
    delay(1500);
  }
  screen(start);                               // end the way the board is held
}

void loop() {}
