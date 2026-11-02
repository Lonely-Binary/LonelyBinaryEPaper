/*
 * 07_Chart - a chart that grows one point at a time.
 *
 * A chart is three things, all from earlier examples: mapping a value to a
 * pixel, drawing a line, and placing text. No charting library - wrapping
 * those three in one hides exactly the part worth learning.
 *
 * !! mapY() is the whole trick !!
 *   Screen y grows DOWNWARD; values grow UPWARD. Every upside-down first chart
 *   comes from getting that one line wrong:
 *       y = bottom - (value - min) * (bottom - top) / (max - min)
 *
 * On e-paper a chart is a good fit for partial refresh: a new point every few
 * seconds is 0.75 s of work and no flashing. When the plot is full, one full
 * refresh clears it (and the ghosting with it) and it starts again.
 *
 * The data is a made-up temperature - a slow wave with some noise - so the
 * sketch runs with nothing attached. Replace reading() with a real sensor.
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

static const int kPoints = 40;              // points across the plot
static const float kMin = 15, kMax = 30;    // the value range on the axis
static float values[kPoints];
static int count = 0;

// Plot area, in pixels. Left margin for the axis labels.
static int16_t L, T, R, B;

static float reading(int i) {
  return 22.5f + 5.0f * sinf(i * 0.25f) + (float)(esp_random() % 100) / 50.0f - 1.0f;
}

static int16_t mapX(int i) { return L + (int32_t)i * (R - L) / (kPoints - 1); }
static int16_t mapY(float v) {
  if (v < kMin) v = kMin;
  if (v > kMax) v = kMax;
  return B - (int16_t)((v - kMin) * (B - T) / (kMax - kMin));
}

static void draw() {
  const int16_t w = display.width();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.setFont(&LB_Font5x8);

  // Axes and three labelled grid lines
  display.drawFastVLine(L, T, B - T + 1, LB_BLACK);
  display.drawFastHLine(L, B, R - L + 1, LB_BLACK);
  for (float v = kMin; v <= kMax; v += 7.5f) {
    const int16_t y = mapY(v);
    for (int16_t x = L; x <= R; x += 4) display.drawPixel(x, y, LB_BLACK);   // dotted
    char buf[8];
    snprintf(buf, sizeof(buf), "%.0f", v);
    display.drawString(buf, L - 4 - display.textWidth(buf), y - 4);
  }

  // The line, then a dot on the newest point
  for (int i = 1; i < count; i++)
    display.drawLine(mapX(i - 1), mapY(values[i - 1]), mapX(i), mapY(values[i]), LB_BLACK);
  if (count) display.fillCircle(mapX(count - 1), mapY(values[count - 1]), 2, LB_BLACK);

  // Title and the latest value, right-aligned
  display.setFont(&LB_Font8x16);
  display.drawString("Temperature", L, 0);
  if (count) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f C", values[count - 1]);
    display.drawString(buf, w - 4 - display.textWidth(buf), 0);
  }
}

void setup() {
  Serial.begin(115200);
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  L = 22; T = 20; R = display.width() - 6; B = display.height() - 8;
  draw();
  display.flush();
}

void loop() {
  delay(2000);
  if (count == kPoints) {                   // full: start over, cleanly
    count = 0;
    draw();
    display.flush();                        // full refresh wipes the ghosting
    return;
  }
  values[count] = reading(count);
  count++;
  draw();
  display.flush(LB_FLUSH_PARTIAL);
  Serial.printf("%2d  %.1f  %s %ld ms\n", count, values[count - 1],
                display.lastWasPartial() ? "partial" : "full", display.lastRefreshMs());
}
