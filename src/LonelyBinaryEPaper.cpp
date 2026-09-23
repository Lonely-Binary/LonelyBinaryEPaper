#include "LonelyBinaryEPaper.h"

constexpr lb_color_t LB_EPDPanel::kInks[2];

// ─── LB_EPDPanel ────────────────────────────────────────────────────────────

// PARTIAL falls back to FULL when there is no previous frame to diff against
// - first refresh, after sleep(), after a timeout. Silently, as the canvas
// contract says (D4); lastWasPartial() reports which one ran.
void LB_EPDPanel::flush(lb_flush_t mode) {
  if (!_epd || !_fb) return;
  if (mode == LB_FLUSH_PARTIAL && _partial && _epd->partialReady()) {
    _lastMs = _epd->refreshPartial(_fb, _stride);
    _lastWasPartial = true;
  } else {
    _lastMs = _epd->refreshFull(_fb, _stride);
    _lastWasPartial = false;
  }
}

void LB_EPDPanel::sleep() {
  if (_epd) _epd->sleep();
}

// ─── LB_EPaper ──────────────────────────────────────────────────────────────

LB_EPaper::~LB_EPaper() {
  delete _epd;
  free(_fb);
}

void LB_EPaper::setWiring(const LB_EPDWiring &wiring) {
  if (_epd) return;  // too late; begin() has already set the pins up
  _wiring = wiring;
  _customWiring = true;
}

bool LB_EPaper::begin() {
  if (_epd) return true;

  _w = _panel->width;
  _h = _panel->height;
  _stride = (_w + 7) / 8;
  _partial = _panel->partialMs > 0;
  const size_t bytes = _stride * _h;
  _fb = (uint8_t *)malloc(bytes);
  if (!_fb) {
    Serial.printf("[LB_EPaper] no room for a %u byte framebuffer.\n", (unsigned)bytes);
    return false;
  }
  memset(_fb, 0xFF, bytes);  // white

  _epd = new LB_EPD(_panel, _wiring);
  // 4 MHz: what the QC rig ran every panel at. An e-paper frame is a few KB
  // and the refresh takes seconds, so a faster clock buys nothing visible.
  if (!_epd->begin(4000000)) {
    delete _epd;
    _epd = nullptr;
    return false;
  }
  LB_Canvas::setRotation(_panel->rotation);
  return true;
}

void LB_EPaper::printInfo(Print &out) const {
  out.println(F("---- Lonely Binary EPaper ----"));
  out.printf("Panel      : %s (%s)\n", _panel->name, _panel->id);
  out.printf("Controller : %s\n", "SSD1680");
  out.printf("Resolution : %dx%d  (native %dx%d, rotation %d)\n",
             width(), height(), _panel->width, _panel->height, rotation());
  if (_panel->partialMs)
    out.printf("Refresh    : full ~%u ms, partial ~%u ms (measured)\n",
               _panel->fullMs, _panel->partialMs);
  else
    out.printf("Refresh    : full ~%u ms, no partial refresh\n", _panel->fullMs);
  out.printf("Pins       : CS=%d DC=%d RST=%d BUSY=%d SCK=%d MOSI=%d%s\n",
             _wiring.cs, _wiring.dc, _wiring.rst, _wiring.busy, _wiring.sclk,
             _wiring.mosi, _customWiring ? "  (custom wiring)" : "");
  if (_epd)
    out.printf("Last flush : %ld ms, %s\n", _lastMs,
               _lastMs == 0 ? "none yet" : _lastWasPartial ? "partial" : "full");
  out.println(F("------------------------------"));
}
