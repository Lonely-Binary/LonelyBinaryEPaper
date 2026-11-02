/*
 * 09_DeepSleep - wake up, draw, sleep. The reason e-paper exists.
 *
 * An e-paper picture needs NO power to stay on the screen. Nothing else does
 * that: a TFT goes black the moment it loses power. So an e-paper device can
 * spend almost all its life asleep - wake up, draw one screen, go back to
 * sleep - and run for months on a battery.
 *
 * This sketch does exactly that every 30 seconds:
 *   wake -> draw -> flush -> ESP32 deep sleep
 * The panel itself already sleeps after every refresh (the library does that),
 * so while the ESP32 sleeps, the whole board is close to off.
 *
 * !! Deep sleep is a restart !!
 *   Waking from deep sleep runs setup() again from the top; loop() never runs.
 *   Ordinary variables start from zero every time. A variable marked
 *   RTC_DATA_ATTR lives in memory that stays powered during deep sleep, so
 *   it survives - that is how the wake-up count below keeps counting.
 *
 * Every wake-up is a full refresh: after a restart the library cannot know
 * what is on the screen, so it does not try a partial one.
 *
 * !! Uploading a new sketch while this one sleeps !!
 *   A sleeping ESP32 has no USB serial. If an upload cannot connect, hold the
 *   BOOT button (IO0), press and release RST, then upload.
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

static const uint32_t kSleepSeconds = 30;

RTC_DATA_ATTR static uint32_t wakeCount = 0;   // survives deep sleep

void setup() {
  Serial.begin(115200);
  wakeCount++;
  const bool fromTimer = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;

  LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  display.begin();
  const int16_t w = display.width(), h = display.height();
  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.drawString("Deep sleep", 6, 4);
  display.drawFastHLine(0, 22, w, LB_BLACK);

  char buf[32];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)wakeCount);
  display.setTextSize(3);
  display.drawString(buf, (w - display.textWidth(buf)) / 2, 34);
  display.setTextSize(1);
  const char *what = wakeCount == 1 ? "first start" : "wake-ups";
  display.drawString(what, (w - display.textWidth(what)) / 2, 68);

  display.setFont(&LB_Font5x8);
  const char *why = fromTimer ? "woken by the timer" : "power-on or reset";
  display.drawString(why, (w - display.textWidth(why)) / 2, 88);
  snprintf(buf, sizeof(buf), "next update in %lu s - unplug me, I stay", (unsigned long)kSleepSeconds);
  display.drawString(buf, (w - display.textWidth(buf)) / 2, h - 12);

  display.flush();
  Serial.printf("wake %lu (%s), refresh %ld ms, sleeping %lu s\n", (unsigned long)wakeCount, why,
                display.lastRefreshMs(), (unsigned long)kSleepSeconds);
  Serial.flush();

  esp_sleep_enable_timer_wakeup((uint64_t)kSleepSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}   // never reached: deep sleep restarts at setup()
