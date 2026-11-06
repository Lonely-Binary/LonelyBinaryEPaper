/*
 * 11_MinuteClock - a desk clock for a colour panel: one full refresh a minute.
 *
 * A colour panel has no partial refresh (see 05_Clock): every update runs the
 * whole three-colour waveform, about 15-19 s on the 2.66" black / white / red
 * panel. That rules out seconds, but a minute is plenty of time - so this
 * clock redraws the whole screen once a minute and makes the most of the red:
 *
 *   left    the time, the date and the weekday on a red card,
 *           and a bar showing how much of the day is gone (red mark = noon)
 *   right   the year, the ESP32-S3's own chip temperature (red above 45 C),
 *           how long it has been running and how many refreshes so far,
 *           and a short line that changes every minute
 *
 * The time starts from when the sketch was compiled and counts on with
 * millis(), so it runs a little behind (the time it took to upload) and starts
 * over after a reset. For real time, connect to Wi-Fi and use configTime()
 * (NTP) - the drawing does not change.
 *
 * The layout is made for the 2.66" panel, 296 x 152 in landscape. It is turned
 * half round (rotation 3) from the panel's default (rotation 1); use
 * setRotation(1) if it comes out upside down on your mounting.
 *
 * Some boards switch the panel's power with a GPIO. Set PIN_EPD_PWR to that pin
 * (and PIN_EPD_PWR_ON to the level that turns it on); -1 = no power switch.
 */
#include <LonelyBinaryEPaper.h>

// ---- GPIO wiring. Every pin this sketch uses is here - change the numbers to match
// ---- your board; nothing below refers to a GPIO by number. -1 = not connected.
#define PIN_CS      10
#define PIN_DC       2
#define PIN_RST     42
#define PIN_BUSY    41   // e-paper BUSY output
#define PIN_SCLK    12
#define PIN_MOSI    11
#define PIN_EPD_PWR -1   // panel power switch, if the board has one
#define PIN_EPD_PWR_ON HIGH
// A board that switches the panel's power on GPIO 47 (tested 2026-11-06):
//   CS 10, DC 21, RST 14, BUSY 38, SCK 12, MOSI 11, PIN_EPD_PWR 47, on = HIGH

struct Now { int y, mo, d, h, mi, wd; };   // here, before any function uses it

LB_EPaper display(LB_266_BWR);

// ---- time, counted on from the compile timestamp ----
static int startY, startMo, startD, startH, startMi, startS;
static uint32_t refreshes = 0;

static int monthFromName(const char *m) {
  static const char *names = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (int i = 0; i < 12; i++)
    if (strncmp(m, names + i * 3, 3) == 0) return i + 1;
  return 1;
}
static bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
static int daysInMonth(int y, int m) {
  static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return (m == 2 && isLeap(y)) ? 29 : d[m - 1];
}
// 0 = Sunday ... 6 = Saturday
static int weekday(int y, int m, int d) {
  static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static Now now() {
  const uint32_t secs = startS + millis() / 1000;
  uint32_t mins = startMi + startH * 60 + secs / 60;
  int y = startY, mo = startMo, d = startD;
  uint32_t days = mins / 1440;
  mins %= 1440;
  while (days--) {
    if (++d > daysInMonth(y, mo)) { d = 1; if (++mo > 12) { mo = 1; y++; } }
  }
  return {y, mo, d, (int)(mins / 60), (int)(mins % 60), weekday(y, mo, d)};
}

static const char *WEEK[7] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
                              "THURSDAY", "FRIDAY", "SATURDAY"};
static const char *MON[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
// At most 16 characters each: the right column is 136 px and the font 8 px wide.
static const char *QUOTES[] = {
  "Zeros to Heroes",
  "Bit by bit",
  "Paper thinks",
  "Ink never sleeps",
  "Keep calm & ink",
  "Black white red",
  "Slow & steady",
  "ESP32-S3 inside",
};

// The built-in font is 8 x 16 pixels at size 1; setTextSize(n) scales both.
static void centred(const char *s, int16_t x, int16_t w, int16_t y) {
  display.drawString(s, x + (w - display.textWidth(s)) / 2, y);
}

static void draw(const Now &t) {
  const int16_t w = display.width(), h = display.height();   // 296 x 152
  char buf[40];
  display.fillScreen(LB_WHITE);

  // ---- left: red time card, 150 x 104 ----
  display.fillRect(0, 0, 150, 104, LB_RED);
  display.setTextColor(LB_WHITE);
  display.setTextSize(3);                                    // 24 x 48 a character
  snprintf(buf, sizeof buf, "%02d:%02d", t.h, t.mi);
  centred(buf, 0, 150, 8);
  display.setTextSize(2);                                    // 16 x 32
  snprintf(buf, sizeof buf, "%02d %s", t.d, MON[t.mo - 1]);
  centred(buf, 0, 150, 56);
  display.setTextSize(1);
  centred(WEEK[t.wd], 0, 150, 86);

  // ---- left, below: how much of the day is gone ----
  const int dayMin = t.h * 60 + t.mi;
  display.setTextColor(LB_BLACK);
  snprintf(buf, sizeof buf, "Day %d%% done", dayMin * 100 / 1440);
  display.drawString(buf, 6, 108);
  display.drawRect(6, 128, 138, 14, LB_BLACK);
  display.fillRect(8, 130, (134 * dayMin) / 1440, 10, LB_BLACK);
  display.fillRect(6 + 69, 124, 2, 22, LB_RED);              // noon

  // ---- right column: x 158 .. 292 ----
  const int16_t x0 = 158, cw = w - x0 - 4;
  display.setTextSize(2);
  snprintf(buf, sizeof buf, "%d", t.y);
  centred(buf, x0, cw, 2);
  display.setTextSize(1);
  centred("Lonely Binary", x0, cw, 32);

  const float c = temperatureRead();                         // ESP32-S3 chip sensor
  display.drawString("CHIP TEMP", x0, 52);
  display.setTextSize(2);
  snprintf(buf, sizeof buf, "%.1f", c);
  display.setTextColor(c >= 45.0f ? LB_RED : LB_BLACK);
  display.drawString(buf, x0 + 4, 66);
  const int16_t tw = display.textWidth(buf);
  display.setTextSize(1);
  display.drawString("o", x0 + 4 + tw + 2, 62);              // degree sign
  display.drawString("C", x0 + 4 + tw + 10, 74);
  display.setTextColor(LB_BLACK);

  const uint32_t up = millis() / 60000;
  snprintf(buf, sizeof buf, "Up %luh%02lum #%lu", (unsigned long)(up / 60),
           (unsigned long)(up % 60), (unsigned long)(refreshes + 1));
  display.drawString(buf, x0, 98);

  // a line of the minute, white on a black band
  const char *q = QUOTES[dayMin % (sizeof QUOTES / sizeof QUOTES[0])];
  display.fillRect(x0 - 4, 118, w - x0 + 4, h - 118, LB_BLACK);
  display.setTextColor(LB_WHITE);
  centred(q, x0, cw, 126);

  display.drawRect(0, 0, w, h, LB_BLACK);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // __DATE__ looks like "Nov  6 2026", __TIME__ like "19:42:05"
  char mon[4] = {0};
  sscanf(__DATE__, "%3s %d %d", mon, &startD, &startY);
  startMo = monthFromName(mon);
  sscanf(__TIME__, "%d:%d:%d", &startH, &startMi, &startS);
  Serial.printf("Clock starts at %04d-%02d-%02d %02d:%02d:%02d (compile time)\n",
                startY, startMo, startD, startH, startMi, startS);

  if (PIN_EPD_PWR >= 0) {          // power the panel before talking to it
    pinMode(PIN_EPD_PWR, OUTPUT);
    digitalWrite(PIN_EPD_PWR, PIN_EPD_PWR_ON);
    delay(100);
  }

  LB_EPDWiring pins = *LB_266_BWR->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);
  if (!display.begin()) {
    Serial.println("begin() failed: not enough RAM for the framebuffer.");
    return;
  }
  display.setRotation(3);          // half a turn from the panel's default
  display.printInfo();
}

void loop() {
  static int lastMinute = -1;
  const Now t = now();
  if (t.mi != lastMinute) {        // a new minute: redraw and refresh once
    lastMinute = t.mi;
    draw(t);
    display.flush();               // ~15-19 s on the 2.66" colour panel
    refreshes++;
    Serial.printf("%02d:%02d drawn, refresh #%lu, %ld ms\n", t.h, t.mi,
                  (unsigned long)refreshes, display.lastRefreshMs());
  }
  delay(500);
}
