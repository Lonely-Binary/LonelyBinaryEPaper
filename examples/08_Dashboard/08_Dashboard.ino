/*
 * 08_Dashboard - four readings, icons, and a layout that is computed.
 *
 * Everything from the earlier examples in one screen: text measured and
 * aligned, icons from Icons.h (copied from 06_Icons), partial refresh for the
 * updates and a full one now and then.
 *
 * !! The layout is arithmetic, not coordinates !!
 *   The screen is split into a 2 x 2 grid from width() and height(), and each
 *   tile lays itself out inside its own rectangle. Change the panel, or the
 *   rotation, and the dashboard rearranges itself. Hard-coded coordinates
 *   would have to be redone by hand for every screen.
 *
 * !! Redraw everything, every time !!
 *   It is tempting to redraw only the tile that changed. On e-paper there is
 *   no gain: a partial refresh costs the same 0.75 s whether one digit moved
 *   or the whole screen did (05_Clock). Redraw the whole frame and let the
 *   panel work out what changed; it cannot get out of step that way.
 *
 * The readings are simulated so the sketch runs with nothing attached.
 */
#include <LonelyBinaryEPaper.h>
#include "Icons.h"

// ---- GPIO wiring. Every pin this sketch uses is here - change the numbers to match
// ---- your board; nothing below refers to a GPIO by number. -1 = not connected.
#define PIN_CS     5
#define PIN_DC    17
#define PIN_RST   16
#define PIN_BUSY   4   // e-paper BUSY output
#define PIN_SCLK  18
#define PIN_MOSI  23

LB_EPaper display(LB_LILYGO_T5_213);

struct Reading {
  const char *label;
  float value;
  const char *unit;
  int kind;   // which icon
};

static Reading readings[4] = {
    {"TEMP", 22.5f, "C", 0},
    {"HUMIDITY", 48, "%", 1},
    {"WEATHER", 0, "", 2},
    {"BATTERY", 87, "%", 3},
};

static void icon(int kind, float v, int16_t cx, int16_t cy, int16_t s) {
  switch (kind) {
    case 0: Icons::thermometer(display, cx, cy, s, (int16_t)((v - 10) * 100 / 30)); break;
    case 1: Icons::drop(display, cx, cy, s); break;
    case 2: if (v > 0.5f) Icons::sun(display, cx, cy, s); else Icons::cloud(display, cx, cy, s); break;
    default: Icons::battery(display, cx, cy, s, (int16_t)v); break;
  }
}

// One tile: icon on the left, label above the value on the right.
static void tile(const Reading &r, int16_t x, int16_t y, int16_t w, int16_t h) {
  display.drawRect(x, y, w, h, LB_BLACK);
  const int16_t s = h - 12;                         // icon fills the tile's height
  icon(r.kind, r.value, x + 4 + s / 2, y + h / 2, s);

  const int16_t tx = x + s + 10;
  display.setFont(&LB_Font5x8);
  display.drawString(r.label, tx, y + 6);
  display.setFont(&LB_Font8x16);

  char buf[16];
  if (r.kind == 2) snprintf(buf, sizeof(buf), "%s", r.value > 0.5f ? "Sunny" : "Cloudy");
  else if (r.value == (int)r.value) snprintf(buf, sizeof(buf), "%d%s", (int)r.value, r.unit);
  else snprintf(buf, sizeof(buf), "%.1f%s", r.value, r.unit);
  display.drawString(buf, tx, y + h - 22);
}

static void draw() {
  const int16_t w = display.width(), h = display.height();
  const int16_t top = 14, gap = 3;
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);

  // Header: title left, uptime right
  display.setFont(&LB_Font5x8);
  display.drawString("Lonely Binary Dashboard", 2, 3);
  char up[16];
  const uint32_t s = millis() / 1000;
  snprintf(up, sizeof(up), "up %02lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60),
           (unsigned long)(s % 60));
  display.drawString(up, w - 2 - display.textWidth(up), 3);

  // 2 x 2 grid, computed from the screen size
  const int16_t tw = (w - gap) / 2, th = (h - top - gap) / 2;
  for (int i = 0; i < 4; i++)
    tile(readings[i], (i % 2) * (tw + gap), top + (i / 2) * (th + gap), tw, th);
}

static void simulate() {
  readings[0].value += (float)((int)(esp_random() % 11) - 5) / 10.0f;   // temp wanders
  readings[1].value = 40 + esp_random() % 20;                            // humidity jumps
  readings[2].value = (millis() / 30000) % 2;                            // weather flips
  if (readings[3].value > 5) readings[3].value -= 1;                     // battery drains
}

void setup() {
  Serial.begin(115200);
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  draw();
  display.flush();
}

void loop() {
  static int updates = 0;
  delay(10000);
  simulate();
  draw();
  // A full refresh every 30 updates (five minutes) wipes the ghosting.
  display.flush(++updates % 30 == 0 ? LB_FLUSH_FULL : LB_FLUSH_PARTIAL);
  Serial.printf("update %d  %s %ld ms\n", updates, display.lastWasPartial() ? "partial" : "full",
                display.lastRefreshMs());
}
