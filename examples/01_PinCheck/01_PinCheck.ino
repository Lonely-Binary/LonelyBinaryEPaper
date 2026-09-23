/*
 * 01_PinCheck - is the panel alive, and is it the size we think? Run this on
 * new hardware, and whenever something looks wrong.
 *
 * Three refreshes, each ruling out a different kind of fault:
 *
 *   1  All black.  2  All white.
 *      The hardest evidence there is. If you see the whole screen turn black
 *      and then white, the panel's power, its waveforms and the SPI link all
 *      work - whatever is wrong later is on the drawing side.
 *
 *   3  Geometry: a frame on the outermost pixels, a block in every corner,
 *      both diagonals, and the size written in the middle.
 *      Solid colours cannot catch a wrong size - all-black looks all-black
 *      whatever the size. Structure can:
 *        a corner block missing          -> the picture is cropped
 *        text mirrored                   -> scan direction is wrong
 *        horizontal streaks, repeating   -> row width is wrong
 *
 * Every refresh is also TIMED, and the time is a diagnosis by itself:
 *   -1 (timeout)                -> BUSY never came back: check BUSY and power
 *   far faster than normal      -> as much a failure as a timeout. The panel
 *                                  did not run its waveform, usually because
 *                                  the data or the commands never arrived
 *   about normal                -> the panel did the work
 * "Normal" for this panel is printed by printInfo().
 */
#include <LonelyBinaryEPaper.h>

LB_EPaper display(LB_LILYGO_T5_213);

static bool verdict(const char *what) {
  const long ms = display.lastRefreshMs();
  const long normal = display.panelDef()->fullMs;
  Serial.printf("%-10s %5ld ms   ", what, ms);
  if (ms < 0) {
    Serial.println("TIMEOUT - BUSY never went idle. Check the BUSY pin and power.");
    return false;
  }
  if (ms < normal / 2) {
    Serial.println("TOO FAST - the panel did not run its refresh. Check DC, CS and MOSI.");
    return false;
  }
  Serial.println("ok");
  return true;
}

static void corner(int16_t x, int16_t y, const char *label, int16_t tx, int16_t ty) {
  display.fillRect(x, y, 8, 8, LB_BLACK);
  display.drawString(label, tx, ty);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n===== 01_PinCheck =====");
  if (!display.begin()) {
    Serial.println("begin() failed: not enough RAM for the framebuffer.");
    return;
  }
  display.printInfo();

  display.fillScreen(LB_BLACK);
  display.flush();
  verdict("1 black");

  display.fillScreen(LB_WHITE);
  display.flush();
  verdict("2 white");

  const int16_t w = display.width(), h = display.height();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.setFont(&LB_Font5x8);
  display.drawRect(0, 0, w, h, LB_BLACK);                 // the outermost pixels
  display.drawLine(0, 0, w - 1, h - 1, LB_BLACK);
  display.drawLine(w - 1, 0, 0, h - 1, LB_BLACK);
  corner(0, 0, "TL", 10, 2);
  corner(w - 8, 0, "TR", w - 22, 2);
  corner(0, h - 8, "BL", 10, h - 10);
  corner(w - 8, h - 8, "BR", w - 22, h - 10);

  char size[16];
  snprintf(size, sizeof(size), "%d x %d", w, h);
  display.setFont(&LB_Font8x16);
  display.setTextColor(LB_BLACK, LB_WHITE);                // opaque: readable over the diagonals
  display.drawString(size, (w - display.textWidth(size)) / 2, h / 2 - 8);
  display.flush();
  verdict("3 geometry");

  Serial.println("Check on the screen: four corner blocks, a complete frame,");
  Serial.println("two diagonals meeting in the middle, the size readable, not mirrored.");
}

void loop() {}
