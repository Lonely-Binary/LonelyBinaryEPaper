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

// See the note at the top of LB_EPD.h. SSD1680: BUSY high = busy.
long LB_EPD::waitBusy(uint32_t timeoutMs) {
  const int busy = HIGH;
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

// Every refresh already ends in deep sleep, and a sleeping SSD1680 ignores
// everything until a hardware reset - so there is no command to send here.
// What sleep() means to a caller is "I will not rely on the previous frame",
// so that is all it records: the next update is a full one.
void LB_EPD::sleep() {
  _partialReady = false;
}
