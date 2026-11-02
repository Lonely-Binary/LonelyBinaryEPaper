/*
 * 10_DeepClean - wipe a ghost off the screen. Run it when an old picture
 * still shows through the new one.
 *
 * Why a ghost appears
 *   An e-paper picture is made of charged ink particles that the panel pushes
 *   to the front or the back of each pixel. Leave the same picture up for a
 *   long time - days, or weeks - and the particles settle where they are. The
 *   next refresh no longer moves all of them, and the old picture stays
 *   faintly visible: a ghost, or "image retention".
 *
 *   Colour panels (black, white and red, or yellow) show it most. The red and
 *   yellow particles are slower than the black ones, so a big block of red or
 *   yellow that stays up for a long time is the classic case: it comes back as
 *   a pink or grey shadow on a white screen. Cold makes it worse.
 *
 * What this sketch does
 *   Fills the whole screen with each ink in turn, several times over:
 *       black -> white -> red -> white      (red or yellow on a colour panel)
 *       black -> white                      (on a black-and-white panel)
 *   and ends on white. Every pixel is driven all the way to each ink, so the
 *   particles that had settled are shaken loose. One round clears a light
 *   ghost; a ghost left for weeks can need the 3 rounds below, or two runs of
 *   the sketch.
 *
 *   It takes time: every full refresh of a colour panel runs the whole colour
 *   waveform. On the 2.13" red panel (14 s a refresh) 3 rounds is about 3
 *   minutes. Leave it alone until the Serial monitor says "done".
 *
 * How to avoid it
 *   - Refresh the screen at least once a day, even when nothing changed.
 *   - Do not leave a large solid block of red or yellow up for days.
 *   - Before putting the screen away, clear it to white (this sketch does).
 *   - In your own sketch: call deepClean() below now and then - once a day,
 *     or every few hundred refreshes.
 *
 * Change LB_213_BWR to your panel (README, "Supported panels") and the pins
 * below to your wiring.
 */
#include <LonelyBinaryEPaper.h>

// ---- GPIO wiring. Every pin this sketch uses is here - change the numbers to match
// ---- your board; nothing below refers to a GPIO by number. -1 = not connected.
#define PIN_CS    10
#define PIN_DC     2
#define PIN_RST   42
#define PIN_BUSY  41   // e-paper BUSY output
#define PIN_SCLK  12
#define PIN_MOSI  11

LB_EPaper display(LB_213_BWR);

static const int kRounds = 3;   // more for a ghost that has been there for weeks

static bool fill(lb_color_t ink, const char *name) {
  display.fillScreen(ink);
  display.flush();
  const long ms = display.lastRefreshMs();
  Serial.printf("  %-6s %6ld ms\n", name, ms);
  if (ms < 0) {
    Serial.println("TIMEOUT - BUSY never went idle. Check the BUSY pin and power.");
    return false;
  }
  return true;
}

/* Every ink, `rounds` times, and white at the end. Copy it into your own
 * sketch as it is: it only needs the display. */
static bool deepClean(int rounds) {
  const LB_EPDPanelDef *panel = display.panelDef();
  const bool colour = panel->inks == 3;
  const lb_color_t third = panel->yellow ? LB_YELLOW : LB_RED;
  const char *thirdName = panel->yellow ? "yellow" : "red";

  for (int r = 1; r <= rounds; r++) {
    Serial.printf("round %d of %d\n", r, rounds);
    if (!fill(LB_BLACK, "black") || !fill(LB_WHITE, "white")) return false;
    if (colour && (!fill(third, thirdName) || !fill(LB_WHITE, "white"))) return false;
  }
  return true;   // the screen is white, and the panel asleep
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n===== 10_DeepClean =====");
  LB_EPDWiring pins = *LB_213_BWR->wiring;   // keeps the SPI bus that fits this chip
  pins.cs = PIN_CS;  pins.dc = PIN_DC;  pins.rst = PIN_RST;
  pins.busy = PIN_BUSY;  pins.sclk = PIN_SCLK;  pins.mosi = PIN_MOSI;
  display.setWiring(pins);

  if (!display.begin()) {
    Serial.println("begin() failed: not enough RAM for the framebuffer.");
    return;
  }
  display.printInfo();

  const LB_EPDPanelDef *panel = display.panelDef();
  const int perRound = panel->inks == 3 ? 4 : 2;
  Serial.printf("%d rounds, %d refreshes, about %ld s. Do not unplug.\n", kRounds,
                kRounds * perRound, (long)kRounds * perRound * panel->fullMs / 1000);

  const uint32_t t0 = millis();
  if (deepClean(kRounds))
    Serial.printf("done in %lu s - the screen is white. Run it again if a ghost is left.\n",
                  (unsigned long)((millis() - t0) / 1000));
}

void loop() {}
