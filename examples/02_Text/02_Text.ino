/*
 * 02_Text - fonts, sizes and alignment.
 *
 * Text is where e-paper shines: it looks like print, it is readable in
 * sunlight, and it stays there with no power.
 *
 * Two fonts ship with the library:
 *   LB_Font8x16   the default. 31 characters across this 250-pixel screen
 *   LB_Font5x8    small, for dense readouts and labels
 * setTextSize(n) scales either by a whole number. Only whole numbers: a bitmap
 * font at 2.5x looks like a mistake.
 *
 * !! Measure, then place !!
 *   Never nudge an x coordinate until the text "looks centred" - it will be
 *   off the moment the text or the font changes. textWidth() tells you how
 *   wide a string is; the rest is arithmetic:
 *       left     x = margin
 *       centre   x = (width - textWidth) / 2
 *       right    x = width - margin - textWidth
 *
 * Text is UTF-8. A character the font does not have is drawn as an empty box,
 * so you can see it is missing rather than get a wrong letter.
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

static void left(const char *s, int16_t y)   { display.drawString(s, 4, y); }
static void centre(const char *s, int16_t y) { display.drawString(s, (display.width() - display.textWidth(s)) / 2, y); }
static void right(const char *s, int16_t y)  { display.drawString(s, display.width() - 4 - display.textWidth(s), y); }

void setup() {
  Serial.begin(115200);
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  const int16_t w = display.width();

  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);

  // 1. The two fonts, one line each
  display.setFont(&LB_Font8x16);
  left("8x16: The quick brown fox", 2);
  display.setFont(&LB_Font5x8);
  left("5x8: The quick brown fox jumps over the lazy dog", 20);

  // 2. Alignment, all three measured
  display.setFont(&LB_Font8x16);
  display.drawFastHLine(0, 31, w, LB_BLACK);
  left("left", 34);
  centre("centre", 34);
  right("right", 34);
  display.drawFastHLine(0, 51, w, LB_BLACK);

  // 3. Scaling: the same font at 1x, 2x, 3x
  display.setTextSize(3);
  display.drawString("23", 4, 58);
  display.setTextSize(2);
  display.drawString("23", 60, 66);
  display.setTextSize(1);
  display.drawString("23", 100, 74);

  // 4. White on black: a filled bar and text drawn on top of it
  display.fillRect(130, 58, w - 130, 48, LB_BLACK);
  display.setTextColor(LB_WHITE);
  display.drawString("inverted", 138, 64);
  display.setFont(&LB_Font5x8);
  display.drawString("white on black", 138, 86);

  // 5. UTF-8: the degree sign is not in the font, so it shows as a box
  display.setTextColor(LB_BLACK);
  display.setFont(&LB_Font8x16);
  left("UTF-8: 21.5\xC2\xB0" "C  (box = missing)", 106);

  display.flush();
  Serial.printf("flush: %ld ms\n", display.lastRefreshMs());
}

void loop() {}
