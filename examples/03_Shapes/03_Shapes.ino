/*
 * 03_Shapes - lines and shapes, and grey on a screen that has no grey.
 *
 * One naming rule runs through the whole API: **draw* outlines, fill* fills**.
 * drawRect/fillRect, drawCircle/fillCircle, drawRoundRect/fillRoundRect,
 * drawTriangle/fillTriangle. Remember that and you rarely need to look
 * anything up. The top row is every draw*, the row below it the matching fill*.
 *
 * !! Black-and-white e-paper has exactly two colours !!
 *   Ask for LB_RED and you get black; ask for light grey and you get white.
 *   Every colour is mapped to the nearer of the two inks. There is no grey
 *   pixel - but there is grey to the eye: black and white pixels mixed in a
 *   pattern, which is how newspapers printed photographs for a century.
 *   The bottom row does exactly that, from 0 to 100 percent black. Look at
 *   it from arm's length and the dots disappear into shades.
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

// A 4x4 pattern with `level` (0..16) black pixels in it, spread as evenly as
// possible - the order in which a 4x4 ordered dither turns pixels black.
static const uint8_t kOrder[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static void shade(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t level) {
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++)
      if (kOrder[(y + j) & 3][(x + i) & 3] < level) display.drawPixel(x + i, y + j, LB_BLACK);
  display.drawRect(x, y, w, h, LB_BLACK);
}

void setup() {
  Serial.begin(115200);
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  display.fillScreen(LB_WHITE);

  // Row 1: outlines
  display.drawRect(4, 4, 36, 30, LB_BLACK);
  display.drawRoundRect(48, 4, 36, 30, 8, LB_BLACK);
  display.drawCircle(107, 19, 15, LB_BLACK);
  display.drawTriangle(130, 33, 148, 4, 166, 33, LB_BLACK);
  display.drawLine(176, 4, 206, 33, LB_BLACK);
  display.drawLine(176, 33, 206, 4, LB_BLACK);

  // Row 2: the same shapes, filled
  display.fillRect(4, 42, 36, 30, LB_BLACK);
  display.fillRoundRect(48, 42, 36, 30, 8, LB_BLACK);
  display.fillCircle(107, 57, 15, LB_BLACK);
  display.fillTriangle(130, 71, 148, 42, 166, 71, LB_BLACK);

  // Colours that are not black or white still land on one of the two
  display.fillRect(176, 42, 14, 30, LB_RED);         // nearer to black
  display.fillRect(194, 42, 14, 30, LB_YELLOW);      // nearer to white: invisible
  display.drawRect(194, 42, 14, 30, LB_BLACK);

  // Row 3: grey by pattern, 0% .. 100% in five steps
  for (int i = 0; i <= 4; i++) shade(4 + i * 44, 82, 40, 30, i * 4);

  display.setFont(&LB_Font5x8);
  display.setTextColor(LB_BLACK);
  display.drawString("draw*", 214, 16);
  display.drawString("fill*", 214, 54);
  display.drawString("R  Y", 178, 74);

  display.flush();
  Serial.printf("flush: %ld ms\n", display.lastRefreshMs());
}

void loop() {}
