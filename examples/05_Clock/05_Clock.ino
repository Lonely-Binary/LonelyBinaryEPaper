/*
 * 05_Clock - a clock, and the fast refresh that makes it work.
 *
 * A full refresh drives every pixel black, then white, then to its final
 * colour. That is the flashing, and it takes about 4 s on the T5. It leaves
 * a perfectly clean picture.
 *
 * A partial refresh only moves the pixels that changed: no flashing, about
 * 0.75 s. That is what a clock wants - fast enough to show SECONDS, and a
 * clock that flashed the whole screen black every second would be unbearable.
 *
 *     display.flush(LB_FLUSH_PARTIAL);
 *
 * !! Not every panel has partial refresh !!
 *   Black-and-white panels do; colour panels do not - every refresh runs the
 *   whole colour waveform, 15 to 45 seconds. Ask the panel, do not assume:
 *       if (display.supportsPartial()) ...
 *   Asking for PARTIAL where there is none is safe (you get a full refresh),
 *   but a sketch that knows can plan for it. This one does:
 *       partial refresh     HH:MM:SS, every second
 *       full refresh only   HH:MM, every minute
 *
 * Three more things about partial refresh:
 *   1  The first flush() is always full. A partial refresh compares against
 *      the picture already on the panel, and at start-up there is none.
 *   2  The time does not depend on how much changed. One digit or the whole
 *      screen, the same 0.75 s - so just redraw the screen and let the panel
 *      work out what moved.
 *   3  It leaves a faint ghost that builds up. A full refresh wipes it; this
 *      clock does one every 30 minutes, on the half hour.
 *
 * The time starts from when the sketch was compiled, and keeps going from
 * there. For real time, connect to Wi-Fi and use configTime() (NTP) - the
 * drawing does not change.
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

// 1 = real time. Set to 60 to watch an hour go by in a minute - handy for
// seeing the full refresh on the half hour without waiting for it (the
// seconds then jump in steps of 60).
static const uint32_t SPEED = 1;

static uint32_t startSeconds;   // time of day at boot, in seconds

static uint32_t now() {         // seconds since midnight
  return (startSeconds + millis() / 1000 * SPEED) % 86400;
}

static bool showSeconds;         // decided once, from the panel

static void draw(uint32_t t) {
  const int16_t w = display.width(), h = display.height();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);

  char buf[32];
  if (showSeconds)
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)(t / 3600),
             (unsigned long)(t / 60 % 60), (unsigned long)(t % 60));
  else
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(t / 3600), (unsigned long)(t / 60 % 60));

  // The largest whole-number size that fits the width. HH:MM:SS at 4x would
  // be 256 px, more than this 250 px screen - measure, never guess.
  uint8_t size = 6;
  do display.setTextSize(size); while (display.textWidth(buf) > w - 8 && --size > 1);
  const int16_t th = display.fontHeight();   // already includes the size
  display.drawString(buf, (w - display.textWidth(buf)) / 2, (h - 32 - th) / 2);
  display.setTextSize(1);

  display.drawFastHLine(10, h - 32, w - 20, LB_BLACK);
  display.drawString(__DATE__, (w - display.textWidth(__DATE__)) / 2, h - 28);

  display.setFont(&LB_Font5x8);
  if (display.supportsPartial())
    snprintf(buf, sizeof(buf), "last: %s %ld ms", display.lastWasPartial() ? "partial" : "full",
             display.lastRefreshMs());
  else
    snprintf(buf, sizeof(buf), "full refresh only");
  display.drawString(buf, w - display.textWidth(buf) - 4, h - 10);
  display.setFont(&LB_Font8x16);
}

void setup() {
  Serial.begin(115200);
  // __TIME__ is "hh:mm:ss", the moment this sketch was compiled
  startSeconds = ((__TIME__[0] - '0') * 10 + (__TIME__[1] - '0')) * 3600 +
                 ((__TIME__[3] - '0') * 10 + (__TIME__[4] - '0')) * 60 +
                 ((__TIME__[6] - '0') * 10 + (__TIME__[7] - '0'));
  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  showSeconds = display.supportsPartial();
  draw(now());
  display.flush();
}

void loop() {
  // What we show changes every second, or every minute without seconds.
  const uint32_t step = showSeconds ? 1 : 60;
  static uint32_t shown = now() / step;
  const uint32_t t = now();
  if (t / step == shown) {
    delay(50);
    return;
  }
  shown = t / step;

  // A full refresh on the hour and the half hour wipes the ghosting. It takes
  // a few seconds, so the seconds skip ahead once every half hour.
  static uint32_t lastFullSlot = t / 1800;
  const bool full = t / 1800 != lastFullSlot;
  lastFullSlot = t / 1800;
  draw(t);
  display.flush(full ? LB_FLUSH_FULL : LB_FLUSH_PARTIAL);
  Serial.printf("%02lu:%02lu:%02lu  %-7s %ld ms\n", (unsigned long)(t / 3600), (unsigned long)(t / 60 % 60),
                (unsigned long)(t % 60), display.lastWasPartial() ? "partial" : "full",
                display.lastRefreshMs());
}
