/*
  LonelyBinaryEPaper — one-line setup for e-paper, with the same drawing API
  as every other Lonely Binary display.

      #include <LonelyBinaryEPaper.h>

      LB_EPaper display(LB_LILYGO_T5_213);   // <- the only line that changes

      void setup() {
        display.begin();
        display.fillScreen(LB_WHITE);
        display.setTextColor(LB_BLACK);
        display.drawString("Hello", 10, 10);
        display.flush();                      // e-paper: THIS is when it draws
      }

  LB_EPaper is an LB_Canvas, so a function written against `LB_Canvas &` runs
  unchanged on a TFT, on VGA and here.

  !! flush() blocks for as long as the panel takes to refresh !!
    About 2 seconds for a full refresh on a black-and-white panel, under one
    for a partial one. Nothing appears before it. Call it once per screen,
    not once per shape.

  MIT License · Lonely Binary
*/
#ifndef LONELY_BINARY_EPAPER_H
#define LONELY_BINARY_EPAPER_H

#include <Arduino.h>
#include <LonelyBinaryGFX.h>

#include "LB_EPD.h"
#include "LB_EPDPanels.h"

/* The LB_Panel adapter: a framebuffer the canvas draws into, and flush()
 * pushes to the panel. Private base of LB_EPaper, like LB_TFTPanel.
 *
 * Black-and-white panels: one 1 bpp buffer the canvas writes directly.
 * Three-ink panels: two 1 bpp planes, black/white and red - the controller's
 * own two RAMs, so flush() sends them as they are. They are two blocks rather
 * than one 2 bpp buffer because on a classic ESP32 without PSRAM there is no
 * single free block of 116 KB for a 7.5" panel, and there are two of 58 KB.
 * The canvas reaches them through writeSpan() (buffer() is nullptr). */
class LB_EPDPanel : public LB_Panel
{
public:
  int16_t panelWidth() const override { return _w; }
  int16_t panelHeight() const override { return _h; }
  lb_format_t format() const override { return _red ? LB_FMT_PAL2 : LB_FMT_PAL1; }
  uint8_t *buffer() const override { return _red ? nullptr : _fb; }
  uint32_t stride() const override { return _stride; }

  /* Index 0 is black and 1 white, because that is what the controller's RAM
   * means by a 0 and a 1 - so the buffer goes to the panel as it is. 2 is red
   * on a three-ink panel. Fixed inks: setPaletteColor() refuses. */
  uint16_t paletteSize() const override { return _red ? 3 : 2; }
  const lb_color_t *palette() const override { return kInks; }
  void writeSpan(int16_t x, int16_t y, int16_t w, lb_color_t index) override;

  /* Per panel: black-and-white panels have partial refresh, colour ones do
   * not (every refresh runs the whole colour waveform). */
  bool supportsPartial() const override { return _partial; }
  void flush(lb_flush_t mode = LB_FLUSH_FULL) override;
  void sleep() override;

protected:
  LB_EPD *_epd = nullptr;
  uint8_t *_fb = nullptr;   // black/white: 1 = white
  uint8_t *_red = nullptr;  // three-ink panels only: 1 = red
  int16_t _w = 0, _h = 0;
  uint32_t _stride = 0;
  long _lastMs = 0;
  bool _lastWasPartial = false;
  bool _partial = false;

private:
  static constexpr lb_color_t kInks[3] = {LB_BLACK, LB_WHITE, LB_RED};
};

class LB_EPaper : private LB_EPDPanel, public LB_Canvas {
 public:
  explicit LB_EPaper(const LB_EPDPanelDef *panel)
      : LB_EPDPanel(), LB_Canvas(*static_cast<LB_EPDPanel *>(this)), _panel(panel),
        _wiring(*panel->wiring) {}
  ~LB_EPaper();

  using LB_Canvas::flush;
  using LB_Canvas::sleep;
  using LB_Canvas::supportsPartial;
  using LB_Canvas::paletteSize;
  using LB_Canvas::setPaletteColor;
  using LB_Canvas::width;
  using LB_Canvas::height;

  /* Your own pins, before begin(). Start from the panel's: */
  //   LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;  pins.busy = 19;
  void setWiring(const LB_EPDWiring &wiring);

  /* Allocate the framebuffer (width x height / 8 bytes: 4 KB for a 2.13";
   * twice that on a three-ink panel: 2 x 58 KB for the 7.5"),
   * start SPI, clear the canvas to white and set the panel's rotation.
   * Does not touch the screen; the first flush() does. */
  bool begin();

  /* How long the last flush() took, in ms (-1 = BUSY timed out), and whether
   * it was a partial refresh. For logs and for sanity: a refresh far faster
   * than normal is as much a failure as a timeout. */
  long lastRefreshMs() const { return _lastMs; }
  bool lastWasPartial() const { return _lastWasPartial; }

  const LB_EPDPanelDef *panelDef() const { return _panel; }
  void printInfo(Print &out = Serial) const;

 private:
  const LB_EPDPanelDef *_panel;
  LB_EPDWiring _wiring;
  bool _customWiring = false;
};

#endif  // LONELY_BINARY_EPAPER_H
