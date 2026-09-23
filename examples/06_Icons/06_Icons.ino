/*
 * 06_Icons - line icons, which is what e-paper is good at.
 *
 * E-paper has two inks and no grey. A photograph has to be broken into dots to
 * survive that, and it never looks like much. A LINE ICON has nothing to lose:
 * it was only ever black strokes on white, and e-paper prints it razor sharp.
 * So on e-paper, draw icons, not pictures.
 *
 * Two ways to make one, both here:
 *
 *   1  Drawn with shapes (Icons.h): a sun is a ring and eight lines, a cloud
 *      is three circles and a rectangle. Any size from the same code, and it
 *      looks the same at 16 pixels as at 64 - the bottom row shows one icon
 *      at three sizes. Nothing to store, nothing to convert.
 *
 *   2  A bitmap: an icon stored as bits, one per pixel, 1 = ink. The heart
 *      below is 16 x 16 = 256 bits = 32 bytes - it is all in this file, you
 *      can see its shape in the binary. Scaling a bitmap means drawing each
 *      bit as a block, so it gets blocky; shapes do not.
 *
 * Icons.h draws on any LB_Canvas, so the same icons work on a Lonely Binary
 * TFT or VGA display.
 */
#include <LonelyBinaryEPaper.h>
#include "Icons.h"

LB_EPaper display(LB_LILYGO_T5_213);

// 16 x 16, one uint16_t per row, the leftmost pixel in the top bit.
static const uint16_t kHeart[16] = {
    0b0000000000000000,
    0b0011100000011100,
    0b0111110000111110,
    0b1111111001111111,
    0b1111111111111111,
    0b1111111111111111,
    0b1111111111111111,
    0b0111111111111110,
    0b0011111111111100,
    0b0001111111111000,
    0b0000111111110000,
    0b0000011111100000,
    0b0000001111000000,
    0b0000000110000000,
    0b0000000000000000,
    0b0000000000000000,
};

static void bitmap(int16_t x, int16_t y, const uint16_t rows[16], int16_t scale) {
  for (int16_t j = 0; j < 16; j++)
    for (int16_t i = 0; i < 16; i++)
      if (rows[j] & (0x8000 >> i)) display.fillRect(x + i * scale, y + j * scale, scale, scale, LB_BLACK);
}

static void label(const char *s, int16_t cx, int16_t y) {
  display.drawString(s, cx - display.textWidth(s) / 2, y);
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.setFont(&LB_Font5x8);

  // Row 1: six icons at 32 px
  const int16_t y1 = 20, s = 32;
  Icons::sun(display, 22, y1, s);            label("sun", 22, y1 + 20);
  Icons::cloud(display, 63, y1, s);          label("cloud", 63, y1 + 20);
  Icons::drop(display, 104, y1, s);          label("rain", 104, y1 + 20);
  Icons::thermometer(display, 145, y1, s, 60); label("temp", 145, y1 + 20);
  Icons::battery(display, 186, y1, s, 75);   label("75%", 186, y1 + 20);
  Icons::wifi(display, 227, y1, s);          label("wifi", 227, y1 + 20);

  // Row 2, left: one icon, three sizes, same code
  display.drawFastHLine(0, 50, display.width(), LB_BLACK);
  Icons::sun(display, 14, 84, 16);
  Icons::sun(display, 44, 84, 24);
  Icons::sun(display, 90, 84, 56);
  label("16  24      56 px", 50, 114);

  // Row 2, right: the bitmap heart at 1x and 3x
  bitmap(136, 76, kHeart, 1);
  bitmap(160, 56, kHeart, 3);
  label("bitmap 1x   3x", 190, 114);

  display.flush();
  Serial.printf("flush: %ld ms\n", display.lastRefreshMs());
}

void loop() {}
