#include "LonelyBinaryEPaper.h"

constexpr lb_color_t LB_EPDPanel::kInks[3];
constexpr lb_color_t LB_EPDPanel::kInksY[3];

// ─── LB_EPDPanel ────────────────────────────────────────────────────────────

// PARTIAL falls back to FULL when there is no previous frame to diff against
// - first refresh, after sleep(), after a timeout. Silently, as the canvas
// contract says (D4); lastWasPartial() reports which one ran.
void LB_EPDPanel::flush(lb_flush_t mode) {
  if (!_epd || !_fb) return;
  if (_red) {
    _lastMs = _epd->refreshColour(_fb, _red, _stride);
    _lastWasPartial = false;
    return;
  }
  if (mode == LB_FLUSH_PARTIAL && _partial && _epd->partialReady()) {
    _lastMs = _epd->refreshPartial(_fb, _stride);
    _lastWasPartial = true;
  } else {
    _lastMs = _epd->refreshFull(_fb, _stride);
    _lastWasPartial = false;
  }
}

// Three-ink panels only (the others hand the canvas their buffer). Each ink is
// a pair of bits: black (0, 0), white (1, 0), red (-, 1).
static void setBits(uint8_t *row, int16_t x, int16_t w, bool on) {
  for (int16_t i = x; i < x + w; i++) {
    const uint8_t m = (uint8_t)(0x80 >> (i & 7));
    if (on) row[i >> 3] |= m; else row[i >> 3] &= (uint8_t)~m;
  }
}

void LB_EPDPanel::writeSpan(int16_t x, int16_t y, int16_t w, lb_color_t index) {
  if (!_red || y < 0 || y >= _h) return;
  if (x < 0) { w += x; x = 0; }
  if (x + w > _w) w = _w - x;
  if (w <= 0) return;
  const size_t off = (size_t)y * _stride;
  setBits(_fb + off, x, w, index != 0);
  setBits(_red + off, x, w, index == 2);
}

void LB_EPDPanel::sleep() {
  if (_epd) _epd->sleep();
}

// ─── LB_EPaper ──────────────────────────────────────────────────────────────

LB_EPaper::~LB_EPaper() {
  delete _epd;
  free(_fb);
  free(_red);
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
  _yellow = _panel->yellow;
  const size_t bytes = _stride * _h;
  _fb = (uint8_t *)malloc(bytes);
  if (!_fb) {
    Serial.printf("[LB_EPaper] no room for a %u byte framebuffer.\n", (unsigned)bytes);
    return false;
  }
  memset(_fb, 0xFF, bytes);  // white
  if (_panel->inks >= 3) {
    _red = (uint8_t *)malloc(bytes);
    if (!_red) {
      Serial.printf("[LB_EPaper] no room for a %u byte red plane.\n", (unsigned)bytes);
      free(_fb);
      _fb = nullptr;
      return false;
    }
    memset(_red, 0x00, bytes);  // no red
  }

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
  out.printf("Controller : %s, %s\n",
             _panel->driver == LB_EPD_UC8179    ? "UC8179"
             : _panel->driver == LB_EPD_UC8176 ? "UC8176"
             : _panel->driver == LB_EPD_UC8159 ? "UC8159"
             : _panel->driver == LB_EPD_UC81XX_Y ? "UC81xx"
             : _panel->driver == LB_EPD_SSD1682 ? "SSD1682"
             : _panel->driver == LB_EPD_SSD1677 ? "SSD1677" : "SSD1680",
             _panel->inks >= 3 ? (_panel->yellow ? "black / white / yellow" : "black / white / red") : "black / white");
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
