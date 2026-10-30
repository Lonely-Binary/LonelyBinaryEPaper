#include "LB_EPD.h"

#include <SPI.h>

// ─── SSD1680 ────────────────────────────────────────────────────────────────
//
// Both cycles follow DKE's sample code for its SSD1680 panels
// (DEPG0213BBS820FxX.c, Epaper_Update_and_Deepsleep_full_update and
// Display_image_new_partial_update). What each step is for, and what happens
// without it, was measured on the QC rig:
//
//   full     hardware reset, SWRESET (0x12), write 0x24 AND 0x26 with the same
//            frame, 0x22 0xF7, 0x20.
//              - 0x26 is the "previous frame" a partial update diffs against.
//                The vendor code writes it here for exactly that reason.
//              - 0xF7 must be sent explicitly: after a partial update the
//                panel's default update mode IS the partial one, and a full
//                refresh without 0x22 runs the partial waveform (837 ms, not
//                2069).
//   partial  hardware reset only - NO SWRESET, it clears RAM and with it the
//            previous frame. 0x3C 0x80, write 0x24 only (writing 0x26 makes
//            the diff zero and nothing changes), 0x22 0xFF, 0x20.
//              - 0xFF runs the panel's own OTP partial waveform. No LUT is
//                loaded from the host.
//
// Both end in deep sleep mode 1 (0x10 0x01), which keeps controller RAM so a
// partial update can follow. The vendor code uses mode 2 (0x03) after a full
// refresh, and SSD1680 does not promise to keep RAM in mode 2; mode 1 costs
// nothing extra. The panel is therefore always asleep between updates - never
// left powered, which is what damages e-paper over time.

static const uint8_t SSD1680_SWRESET = 0x12;
static const uint8_t SSD1680_RAM_BW = 0x24;     // the frame to show
static const uint8_t SSD1680_RAM_PREV = 0x26;   // the frame on screen now
static const uint8_t SSD1680_UPDATE_CTRL2 = 0x22;
static const uint8_t SSD1680_ACTIVATE = 0x20;
static const uint8_t SSD1680_BORDER = 0x3C;
static const uint8_t SSD1680_DEEP_SLEEP = 0x10;
static const uint8_t SSD1680_DRIVER_OUTPUT = 0x01;
static const uint8_t SSD1680_DATA_ENTRY = 0x11;
static const uint8_t SSD1680_RAM_X_RANGE = 0x44;
static const uint8_t SSD1680_RAM_Y_RANGE = 0x45;
static const uint8_t SSD1680_RAM_X_COUNTER = 0x4E;
static const uint8_t SSD1680_RAM_Y_COUNTER = 0x4F;
static const uint8_t SSD1680_TEMP_SENSOR = 0x18;

// Set the RAM window to exactly the frame we send, instead of trusting the
// power-on defaults.
//
// The vendor sample leaves this block commented out and relies on the
// defaults, and on the DKE panel in the QC rig that worked. On the LilyGO T5
// it did not: the default X window is 13 bytes a row, we send 16, and every
// row landed a few bytes further along than the last - the whole screen came
// out as horizontal streaks repeated four times across, with the picture
// still faintly visible in them. That is what a row-width mismatch looks
// like; a wrong size merely crops.
//
// A hardware reset puts these registers back to their defaults (RAM itself
// survives), so this runs before every write, partial ones included.

LB_EPD::~LB_EPD() {
  if (_spi) {
    _spi->end();
    delete _spi;
  }
}

bool LB_EPD::begin(int32_t spiHz) {
  _hz = spiHz;
  pinMode(_w.cs, OUTPUT);
  digitalWrite(_w.cs, HIGH);
  pinMode(_w.dc, OUTPUT);
  digitalWrite(_w.dc, HIGH);
  pinMode(_w.rst, OUTPUT);
  digitalWrite(_w.rst, HIGH);
  pinMode(_w.busy, INPUT);
  _spi = new SPIClass(_w.spiHost);
  if (!_spi) return false;
  // MISO is -1 on purpose: on VSPI the default MISO is GPIO 19, which is BUSY
  // on some of our boards, and the SPI peripheral would fight it.
  _spi->begin(_w.sclk, -1, _w.mosi, -1);
  return true;
}

void LB_EPD::cmd(uint8_t c) {
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_w.dc, LOW);
  digitalWrite(_w.cs, LOW);
  _spi->write(c);
  digitalWrite(_w.cs, HIGH);
  digitalWrite(_w.dc, HIGH);
  _spi->endTransaction();
}

void LB_EPD::dat(uint8_t d) {
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_w.cs, LOW);
  _spi->write(d);
  digitalWrite(_w.cs, HIGH);
  _spi->endTransaction();
}

// Rows go out in scan order. On a panel whose default scan runs the long axis
// backwards, sending the rows last-first puts the picture the right way round
// at no cost - instead of mirroring every pixel as it is drawn.
void LB_EPD::sendFrame(uint8_t ramCmd, const uint8_t *frame, uint32_t stride) {
  cmd(ramCmd);
  const uint16_t rows = _panel->height;
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_w.cs, LOW);
  for (uint16_t r = 0; r < rows; r++) {
    const uint16_t src = _panel->mirrorGate ? rows - 1 - r : r;
    _spi->writeBytes(frame + (size_t)src * stride, stride);
  }
  digitalWrite(_w.cs, HIGH);
  _spi->endTransaction();
}

// Timings as on the QC rig's direct driver, which the vendor's 20 ms / 20 ms
// also satisfies. The reset itself is short; the wait after it is BUSY's job.
void LB_EPD::hardwareReset() {
  delay(10);
  digitalWrite(_w.rst, HIGH);
  delay(20);
  digitalWrite(_w.rst, LOW);
  delay(20);
  digitalWrite(_w.rst, HIGH);
  delay(20);
}

// See the note at the top of LB_EPD.h. Solomon (SSD16xx): BUSY high = busy.
// UltraChip (UC8179): the other way round - its pin is BUSY_N.
long LB_EPD::waitBusy(uint32_t timeoutMs) {
  const int busy = (_panel->driver == LB_EPD_UC8179 || _panel->driver == LB_EPD_UC8176 || _panel->driver == LB_EPD_UC8159 || _panel->driver == LB_EPD_UC81XX_Y) ? LOW : HIGH;
  const uint32_t t0 = millis();
  while (digitalRead(_w.busy) != busy) {
    if (millis() - t0 > 200) break;  // never went busy: already done
    delayMicroseconds(200);
  }
  while (digitalRead(_w.busy) == busy) {
    if (millis() - t0 > timeoutMs) return -1;
    delay(1);
  }
  return (long)(millis() - t0);
}

void LB_EPD::setWindow(uint32_t stride) {
  // The built-in temperature sensor, not an external one. SSD1680 picks the
  // refresh waveform by temperature, and its default is to read a sensor on
  // its I2C pins - which the T5 does not have. With the reading it got
  // instead, the waveform was too weak: an old screen was not wiped (each new
  // one was drawn on top of the last), solid black came out speckled, and a
  // second refresh in a row came out faint. The vendor sample for the DKE
  // panel does not send this, so its OTP presumably defaults to internal.
  cmd(SSD1680_TEMP_SENSOR);
  dat(0x80);

  const uint16_t lastRow = _panel->height - 1;
  cmd(SSD1680_DRIVER_OUTPUT);  // gate count - 1, then scan order (default)
  dat(lastRow & 0xFF);
  dat(lastRow >> 8);
  dat(0x00);
  cmd(SSD1680_DATA_ENTRY);     // X increment, Y increment
  dat(0x03);
  cmd(SSD1680_RAM_X_RANGE);    // in bytes
  dat(0x00);
  dat(stride - 1);
  cmd(SSD1680_RAM_Y_RANGE);    // in rows
  dat(0x00);
  dat(0x00);
  dat(lastRow & 0xFF);
  dat(lastRow >> 8);
  cmd(SSD1680_RAM_X_COUNTER);
  dat(0x00);
  cmd(SSD1680_RAM_Y_COUNTER);
  dat(0x00);
  dat(0x00);
}

long LB_EPD::refreshFull(const uint8_t *frame, uint32_t stride) {
  if (_panel->driver == LB_EPD_UC8176) return refreshUC8176(frame, stride);
  // A timeout well past the measured time: long enough for a cold panel,
  // short enough that a dead one does not hang the sketch for a minute.
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;

  hardwareReset();
  waitBusy(3000);
  cmd(SSD1680_SWRESET);
  waitBusy(3000);
  setWindow(stride);

  sendFrame(SSD1680_RAM_BW, frame, stride);
  setWindow(stride);  // the address counters ran to the end with that frame
  sendFrame(SSD1680_RAM_PREV, frame, stride);

  cmd(SSD1680_UPDATE_CTRL2);
  dat(0xF7);
  cmd(SSD1680_ACTIVATE);
  const long ms = waitBusy(timeout);

  cmd(SSD1680_DEEP_SLEEP);
  dat(0x01);
  _partialReady = ms >= 0;
  return ms;
}

long LB_EPD::refreshPartial(const uint8_t *frame, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->partialMs * 4;

  hardwareReset();
  waitBusy(3000);
  cmd(SSD1680_BORDER);
  dat(0x80);
  setWindow(stride);

  sendFrame(SSD1680_RAM_BW, frame, stride);

  cmd(SSD1680_UPDATE_CTRL2);
  dat(0xFF);
  cmd(SSD1680_ACTIVATE);
  const long ms = waitBusy(timeout);

  cmd(SSD1680_DEEP_SLEEP);
  dat(0x01);
  if (ms < 0) _partialReady = false;
  return ms;
}

// Three-colour SSD1680 (2.13" 122 x 250, GoodDisplay GDEY0213Z98). Same chip as
// the black-and-white panels above, with the colour differences from the
// vendor's demo: border waveform 0x3C 0x05, display update control 0x21 0x00
// 0x80 (black/white and red RAM both enabled), then 0x24 = black/white
// (1 = white) and 0x26 = red (1 = red), and the full-update mode 0xF7. One
// refresh runs the three-colour waveform: about 15 s, whatever the content.

long LB_EPD::refreshSSD1680Colour(const uint8_t *bw, const uint8_t *red, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;

  hardwareReset();
  waitBusy(3000);
  cmd(SSD1680_SWRESET);
  waitBusy(3000);
  setWindow(stride);
  cmd(SSD1680_BORDER);
  dat(0x05);
  if (_panel->driver == LB_EPD_SSD1680) {  // SSD1682 (152-gate panels) is left at its default
    cmd(0x21);
    dat(0x00);
    dat(0x80);
  }

  sendFrame(SSD1680_RAM_BW, bw, stride);
  setWindow(stride);
  sendFrame(SSD1680_RAM_PREV, red, stride);

  cmd(SSD1680_UPDATE_CTRL2);
  dat(0xF7);
  cmd(SSD1680_ACTIVATE);
  const long ms = waitBusy(timeout);

  cmd(SSD1680_DEEP_SLEEP);
  dat(0x01);
  _partialReady = false;
  return ms;
}

// ─── SSD1677 ────────────────────────────────────────────────────────────────
//
// Waveshare's EPD_7in5b_HD.c (the 880 x 528 black / white / red panel, sold
// by GoodDisplay as GDEH075Z90 and fitted to HINK-E075A07), step for step:
//
//   reset, SWRESET, auto-fill both RAMs (0x46 / 0x47), booster soft start,
//   688 gates scanned bottom-up, X increment / Y decrement with Y starting at
//   687, internal temperature sensor, load the waveform (0x22 0xB1), then
//   0x24 = black/white (1 = white) and 0x26 = red (1 = red), 0x22 0xC7, 0x20.
//
// X addresses are in PIXELS on this controller (0..879), not bytes as on the
// SSD1680. The 688 gate lines and the Y range starting at 687 are the vendor's
// values for a 528-row panel; kept as they are until the panel says otherwise.

static const uint16_t SSD1677_GATES = 688;

void LB_EPD::cursorSSD1677() {
  cmd(SSD1680_RAM_X_COUNTER);
  dat(0x00);
  dat(0x00);
  cmd(SSD1680_RAM_Y_COUNTER);
  dat((SSD1677_GATES - 1) & 0xFF);
  dat((SSD1677_GATES - 1) >> 8);
}

void LB_EPD::initSSD1677() {
  hardwareReset();
  waitBusy(3000);
  cmd(SSD1680_SWRESET);
  waitBusy(3000);

  cmd(0x46);  // auto-write red RAM
  dat(0xF7);
  waitBusy(3000);
  cmd(0x47);  // auto-write black/white RAM
  dat(0xF7);
  waitBusy(3000);

  cmd(0x0C);  // booster soft start
  dat(0xAE);
  dat(0xC7);
  dat(0xC3);
  dat(0xC0);
  dat(0x40);

  cmd(SSD1680_DRIVER_OUTPUT);  // 688 gates, scanned bottom-up
  dat((SSD1677_GATES - 1) & 0xFF);
  dat((SSD1677_GATES - 1) >> 8);
  dat(0x01);
  cmd(SSD1680_DATA_ENTRY);     // X increment, Y decrement
  dat(0x01);

  const uint16_t lastX = _panel->width - 1;
  cmd(SSD1680_RAM_X_RANGE);    // in pixels
  dat(0x00);
  dat(0x00);
  dat(lastX & 0xFF);
  dat(lastX >> 8);
  cmd(SSD1680_RAM_Y_RANGE);    // from the top gate down
  dat((SSD1677_GATES - 1) & 0xFF);
  dat((SSD1677_GATES - 1) >> 8);
  dat(0x00);
  dat(0x00);

  cmd(SSD1680_BORDER);         // border waveform
  dat(0x01);
  cmd(SSD1680_TEMP_SENSOR);    // internal sensor
  dat(0x80);
  cmd(SSD1680_UPDATE_CTRL2);   // load temperature and waveform
  dat(0xB1);
  cmd(SSD1680_ACTIVATE);
  waitBusy(3000);
  cursorSSD1677();
}

long LB_EPD::refreshColour(const uint8_t *bw, const uint8_t *red, uint32_t stride) {
  if (_panel->driver == LB_EPD_UC8179) return refreshUC8179(bw, red, stride);
  if (_panel->driver == LB_EPD_UC8159) return refreshUC8159(bw, red, stride);
  if (_panel->driver == LB_EPD_UC81XX_Y) return refreshUC81xxY(bw, red, stride);
  if (_panel->driver == LB_EPD_SSD1680 || _panel->driver == LB_EPD_SSD1682) return refreshSSD1680Colour(bw, red, stride);
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;

  initSSD1677();
  sendFrame(SSD1680_RAM_BW, bw, stride);
  cursorSSD1677();
  sendFrame(SSD1680_RAM_PREV, red, stride);  // on a colour panel, 0x26 is red

  cmd(SSD1680_UPDATE_CTRL2);
  dat(0xC7);
  cmd(SSD1680_ACTIVATE);
  const long ms = waitBusy(timeout);

  cmd(SSD1680_DEEP_SLEEP);
  dat(0x01);
  _partialReady = false;
  return ms;
}

// ─── UC8179 ─────────────────────────────────────────────────────────────────
//
// Waveshare's EPD_7in5b_V2.c (800 x 480 black / white / red), step for step:
//
//   reset, power setting (0x01), booster soft start (0x06), POWER ON (0x04)
//   and wait, panel setting 0x00 0x0F (black/white/red, waveform from OTP),
//   resolution 800 x 480 (0x61), 0x15 0x00, VCOM / data interval (0x50 0x11
//   0x07), TCON (0x60 0x22); then 0x10 = black/white (1 = white), 0x13 = red
//   (1 = red), DISPLAY REFRESH (0x12), and power off + deep sleep (0x07 0xA5).
//
// A completely different command set from the Solomon chips; only SPI, reset
// and the two 1 bpp planes are shared. A sleeping UC8179 wakes only on reset.

long LB_EPD::refreshUC8179(const uint8_t *bw, const uint8_t *red, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;

  hardwareReset();
  waitBusy(3000);
  cmd(0x01);  // power setting: VGH/VGL +-20 V, VDH/VDL +-15 V
  dat(0x07);
  dat(0x07);
  dat(0x3F);
  dat(0x3F);
  cmd(0x06);  // booster soft start
  dat(0x17);
  dat(0x17);
  dat(0x28);
  dat(0x17);
  cmd(0x04);  // power on
  delay(100);
  waitBusy(3000);
  cmd(0x00);  // panel setting: black/white/red, waveform from OTP
  dat(0x0F);
  cmd(0x61);  // resolution
  dat(_panel->width >> 8);
  dat(_panel->width & 0xFF);
  dat(_panel->height >> 8);
  dat(_panel->height & 0xFF);
  cmd(0x15);  // dual SPI off
  dat(0x00);
  cmd(0x50);  // VCOM and data interval
  dat(0x11);
  dat(0x07);
  cmd(0x60);  // TCON
  dat(0x22);

  sendFrame(0x10, bw, stride);
  sendFrame(0x13, red, stride);

  cmd(0x12);  // display refresh
  delay(10);  // the vendor: at least 200 us before BUSY means anything
  const long ms = waitBusy(timeout);

  cmd(0x50);  // border floating while asleep, as the vendor does
  dat(0xF7);
  cmd(0x02);  // power off
  waitBusy(3000);
  cmd(0x07);  // deep sleep
  dat(0xA5);
  _partialReady = false;
  return ms;
}

// ─── UC8159 ─────────────────────────────────────────────────────────────────
//
// 7.5" 640 x 384 black / white / red, the generation before UC8179. Waveshare's
// EPD_7in5bc.c (MIT): power 0x37 0x00, panel 0xCF 0x08, PLL 0x3A, VCOM 0x28,
// booster C7 CC 15, data interval 0x77, TCON 0x22, flash off (0x65 0x00),
// resolution, flash mode 0x03 (0xE5). One data stream (0x10), 4 bits a pixel,
// left pixel in the high nibble: 0 black, 3 white, 4 red. Power on comes
// after the data, then refresh, then power off and deep sleep.

long LB_EPD::refreshUC8159(const uint8_t *bw, const uint8_t *red, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;
  const uint16_t w = _panel->width, h = _panel->height;

  hardwareReset();
  delay(200);
  waitBusy(3000);
  cmd(0x01);  // power setting
  dat(0x37);
  dat(0x00);
  cmd(0x00);  // panel setting
  dat(0xCF);
  dat(0x08);
  cmd(0x30);  // PLL
  dat(0x3A);
  cmd(0x82);  // VCOM DC
  dat(0x28);
  cmd(0x06);  // booster soft start
  dat(0xC7);
  dat(0xCC);
  dat(0x15);
  cmd(0x50);  // VCOM and data interval
  dat(0x77);
  cmd(0x60);  // TCON
  dat(0x22);
  cmd(0x65);  // flash control: off
  dat(0x00);
  cmd(0x61);  // resolution
  dat(w >> 8);
  dat(w & 0xFF);
  dat(h >> 8);
  dat(h & 0xFF);
  cmd(0xE5);  // flash mode
  dat(0x03);

  // Row by row: two planes in, one nibble stream out, one row buffer.
  cmd(0x10);
  uint8_t row[400];
  const uint16_t outBytes = w / 2;
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_w.cs, LOW);
  for (uint16_t y = 0; y < h; y++) {
    const uint16_t src = _panel->mirrorGate ? h - 1 - y : y;
    const uint8_t *b = bw + (size_t)src * stride, *r = red + (size_t)src * stride;
    for (uint16_t x = 0; x < w; x += 2) {
      uint8_t out = 0;
      for (uint8_t k = 0; k < 2; k++) {
        const uint16_t px = x + k;const uint8_t m = 0x80 >> (px & 7);
        const uint8_t v = (r[px >> 3] & m) ? 0x4 : (b[px >> 3] & m) ? 0x3 : 0x0;
        out = (out << 4) | v;
      }
      row[x / 2] = out;
    }
    _spi->writeBytes(row, outBytes);
  }
  digitalWrite(_w.cs, HIGH);
  _spi->endTransaction();

  cmd(0x04);  // power on
  delay(100);
  waitBusy(3000);
  cmd(0x12);  // display refresh
  delay(100);
  const long ms = waitBusy(timeout);

  cmd(0x02);  // power off
  waitBusy(3000);
  cmd(0x07);  // deep sleep
  dat(0xA5);
  _partialReady = false;
  return ms;
}

// ─── UC8176 ─────────────────────────────────────────────────────────────────
//
// 4.2" 400 x 300 black and white (GoodDisplay GDEW042T2, Waveshare 4.2" V1,
// ribbon WFT0420CZ15). Waveshare's EPD_4in2.c for the power, booster, PLL,
// resolution and VCOM values; the waveform is the panel's own OTP one (panel
// setting 0x1F, "BWOTP" in their notes) rather than their host-loaded LUTs.
// 0x10 is the old frame and 0x13 the new; both get the new frame, so the
// full-refresh waveform drives every pixel from a known state.

long LB_EPD::refreshUC8176(const uint8_t *bw, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->fullMs * 4;

  hardwareReset();
  delay(200);  // UC81xx wants time after reset before BUSY means anything (QC notes)
  waitBusy(3000);
  cmd(0x01);  // power setting
  dat(0x03);
  dat(0x00);
  dat(0x2B);
  dat(0x2B);
  cmd(0x06);  // booster soft start
  dat(0x17);
  dat(0x17);
  dat(0x17);
  cmd(0x04);  // power on
  delay(100);
  waitBusy(3000);
  cmd(0x00);  // panel setting: black/white, waveform from OTP
  dat(0x1F);
  cmd(0x30);  // PLL, 50 Hz
  dat(0x3C);
  cmd(0x61);  // resolution
  dat(_panel->width >> 8);
  dat(_panel->width & 0xFF);
  dat(_panel->height >> 8);
  dat(_panel->height & 0xFF);
  cmd(0x82);  // VCOM DC
  dat(0x12);
  cmd(0x50);  // VCOM and data interval
  dat(0x97);

  sendFrame(0x10, bw, stride);
  sendFrame(0x13, bw, stride);

  cmd(0x12);  // display refresh
  delay(10);
  const long ms = waitBusy(timeout);

  cmd(0x50);
  dat(0xF7);
  cmd(0x02);  // power off
  waitBusy(3000);
  cmd(0x07);  // deep sleep
  dat(0xA5);
  _partialReady = false;
  return ms;
}

// ─── UC81xx black / white / yellow (4.37" 176 x 480) ────────────────────────
//
// Probed on the panel (QC repository, DEPG0437): no datasheet, no vendor code.
//   - NO init registers. 0x61 TRES hangs power-on; 0x50 CDI hangs the refresh;
//     0x00 PSR flips the picture. The OTP configuration is right as it is.
//   - 0x10 = black/white (1 = white), 0x13 = colour (1 = yellow), then power on
//     (0x04), refresh (0x12, NO parameter byte), power off (0x02), deep sleep
//     (0x07 0xA5). BUSY low = busy. A full refresh takes about 25 s.
//   - The source axis is backwards: each row goes out last byte first and every
//     byte bit-reversed, or the whole page comes out mirrored.

void LB_EPD::sendPlaneReversed(uint8_t ramCmd, const uint8_t *plane, uint32_t stride) {
  cmd(ramCmd);
  uint8_t row[48];  // 176 / 8 = 22 bytes a row
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_w.cs, LOW);
  for (uint16_t y = 0; y < _panel->height; y++) {
    const uint16_t src = _panel->mirrorGate ? _panel->height - 1 - y : y;
    const uint8_t *in = plane + (size_t)src * stride;
    for (uint32_t i = 0; i < stride; i++) {
      uint8_t b = in[stride - 1 - i];
      b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
      b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
      b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
      row[i] = b;
    }
    _spi->writeBytes(row, stride);
  }
  digitalWrite(_w.cs, HIGH);
  _spi->endTransaction();
}

long LB_EPD::refreshUC81xxY(const uint8_t *bw, const uint8_t *colour, uint32_t stride) {
  const uint32_t timeout = (uint32_t)_panel->fullMs * 2;

  // The probe's reset: RST high 20 ms, low 10 ms, high 50 ms.
  digitalWrite(_w.rst, HIGH);
  delay(20);
  digitalWrite(_w.rst, LOW);
  delay(10);
  digitalWrite(_w.rst, HIGH);
  delay(50);

  sendPlaneReversed(0x10, bw, stride);
  sendPlaneReversed(0x13, colour, stride);

  cmd(0x04);  // power on
  waitBusy(3000);
  cmd(0x12);  // display refresh - no parameter
  const long ms = waitBusy(timeout);
  cmd(0x02);  // power off
  waitBusy(3000);
  cmd(0x07);  // deep sleep
  dat(0xA5);
  _partialReady = false;
  return ms;
}

// Every refresh already ends in deep sleep, and a sleeping SSD1680 ignores
// everything until a hardware reset - so there is no command to send here.
// What sleep() means to a caller is "I will not rely on the previous frame",
// so that is all it records: the next update is a full one.
void LB_EPD::sleep() {
  _partialReady = false;
}
