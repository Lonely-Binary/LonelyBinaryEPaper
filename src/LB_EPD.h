#pragma once
/*
 * LB_EPD - the e-paper panel driver: SPI, BUSY, reset, and the refresh cycles.
 *
 * The sequences come from the panel vendor's own sample code (DKE, for the
 * SSD1680 panels) and from the direct drivers verified on real panels in the
 * QC repository - never from GxEPD2, which is GPL-3.0. Every rule below was
 * learned on a real panel; the notes say which.
 *
 * !! Waiting for BUSY is two waits, not one !!
 *   There is a delay between a command and BUSY going busy. A loop that only
 *   waits for "not busy" returns in 0 ms, and the next commands land on a
 *   chip that is still resetting and are swallowed whole. The symptom is a
 *   refresh that takes the right time and changes nothing. So: wait for it to
 *   become busy (briefly - it may already be done), THEN for it to finish.
 */
#include <Arduino.h>
#include "LB_EPDPanels.h"

class LB_EPD
{
public:
  LB_EPD(const LB_EPDPanelDef *panel, const LB_EPDWiring &wiring)
      : _panel(panel), _w(wiring) {}
  ~LB_EPD();

  bool begin(int32_t spiHz);

  /* Push `frame` (native orientation, 1 bpp, 1 = white, MSB leftmost,
   * `stride` bytes per row) and refresh. Blocks until the panel is done.
   * Returns the refresh time in ms, or -1 on a BUSY timeout. */
  long refreshFull(const uint8_t *frame, uint32_t stride);

  /* Differential refresh against the frame on screen. Only valid after a
   * full refresh and before sleep(); the caller checks partialReady(). */
  long refreshPartial(const uint8_t *frame, uint32_t stride);

  /* Three-ink panels: `bw` is 1 = white / 0 = black, `red` is 1 = red (red
   * wins over black). Four-ink panels: the pair (bw, red) is the ink -
   * (0, 0) black, (1, 0) white, (0, 1) red, (1, 1) yellow. Both native orientation, `stride` bytes a row. Always a
   * full refresh. Returns the refresh time in ms, or -1 on a BUSY timeout. */
  long refreshColour(const uint8_t *bw, const uint8_t *red, uint32_t stride);

  bool partialReady() const { return _partialReady; }

  /* The panel sleeps after every refresh anyway; this only means the next
   * update has to be a full one. */
  void sleep();

private:
  const LB_EPDPanelDef *_panel;
  LB_EPDWiring _w;
  int32_t _hz = 4000000;
  class SPIClass *_spi = nullptr;
  bool _partialReady = false;

  void cmd(uint8_t c);
  void dat(uint8_t d);
  void sendFrame(uint8_t ramCmd, const uint8_t *frame, uint32_t stride);
  void hardwareReset();
  void setWindow(uint32_t stride);
  void initSSD1677();
  void cursorSSD1677();
  long refreshUC8179(const uint8_t *bw, const uint8_t *red, uint32_t stride);
  long refreshUC8176(const uint8_t *bw, uint32_t stride);
  long refreshUC8176Colour(const uint8_t *bw, const uint8_t *red, uint32_t stride);
  long refreshJD79653A(const uint8_t *bw, uint32_t stride);
  void initJD79653A();
  long refreshUC8159(const uint8_t *bw, const uint8_t *red, uint32_t stride);
  long refreshSSD1680Colour(const uint8_t *bw, const uint8_t *red, uint32_t stride);
  long refreshUC81xxY(const uint8_t *bw, const uint8_t *colour, uint32_t stride);
  long refreshUC81xxRY(const uint8_t *bw, const uint8_t *colour, uint32_t stride);
  void sendPlaneReversed(uint8_t ramCmd, const uint8_t *plane, uint32_t stride);
  long waitBusy(uint32_t timeoutMs);
};
