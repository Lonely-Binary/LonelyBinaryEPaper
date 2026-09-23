/*
  HelloEPaper — the Lonely Binary drawing API on e-paper.

  Draws a screen with a full refresh, then updates a counter once a second
  with partial refreshes, which are about three times faster and do not
  flash the whole screen.

  TO USE A DIFFERENT PANEL, CHANGE ONE LINE — the LB_* constant below.

  Tools > Board: "ESP32 Dev Module".

  Two things that are different from a TFT:
    - Nothing appears until flush(). Draw the whole screen, then flush once.
    - flush() waits for the panel: about 2.3 s for a full refresh on the
      LilyGO T5, 0.7 s for a partial one.
*/
#include <LonelyBinaryEPaper.h>

LB_EPaper display(LB_LILYGO_T5_213);

static int count = 0;

static void drawCount() {
  const int16_t w = display.width(), h = display.height();
  display.fillRect(0, 48, w, 50, LB_WHITE);   // clear the old number first
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", count);
  display.setTextSize(4);
  display.drawString(buf, (w - display.textWidth(buf)) / 2, 56);
  display.setTextSize(1);
  (void)h;
}

void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.println("Display did not start.");
    return;
  }
  display.printInfo();

  display.fillScreen(LB_WHITE);
  display.fillRect(0, 0, display.width(), 20, LB_BLACK);
  display.setTextColor(LB_WHITE);
  display.drawString("Hello, e-paper", 6, 6);
  display.setTextColor(LB_BLACK);
  display.drawString("partial refresh, once a second", 6, 106);
  drawCount();
  display.flush();                  // full refresh: the first one always is
}

void loop() {
  delay(1000);
  count++;
  drawCount();
  // Every 30th update is a full one. Partial refreshes leave a faint ghost
  // that builds up; a full refresh wipes it.
  display.flush(count % 30 == 0 ? LB_FLUSH_FULL : LB_FLUSH_PARTIAL);
  Serial.printf("%d  %s %ld ms\n", count, display.lastWasPartial() ? "partial" : "full",
                display.lastRefreshMs());
}
