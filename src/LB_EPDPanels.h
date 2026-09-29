#pragma once
/*
 * LB_EPDPanels.h - the e-paper panels this library drives.
 *
 * One entry per panel. Everything that differs between two panels on the same
 * controller is here; the refresh sequences themselves are in LB_EPD.cpp.
 *
 * (Hand-written for now, while the table is short. It moves to a panels.yaml
 * and a generator, as in Lonely Binary Display, when it grows.)
 */
#include <Arduino.h>  // VSPI / HSPI

enum LB_EPDDriver : uint8_t {
  LB_EPD_SSD1680,   // Solomon SSD1680 / SSD168x black-and-white: BUSY high = busy
  LB_EPD_SSD1677,   // Solomon SSD1677, large panels (7.5" 880 x 528): BUSY high = busy
  LB_EPD_UC8179,    // UltraChip UC8179 (7.5" 800 x 480): BUSY LOW = busy
  LB_EPD_UC8176,    // UltraChip UC8176 (4.2" 400 x 300, black and white): BUSY LOW = busy
  LB_EPD_UC8159,    // UltraChip UC8159 (7.5" 640 x 384 three-ink, 4 bits a pixel): BUSY LOW = busy
};

/* The pins. A board with the panel built in carries its own; a bare panel on
 * our breakout would use the kit wiring for the MCU instead. */
struct LB_EPDWiring {
  int8_t cs, dc, rst, busy, sclk, mosi;
  uint8_t spiHost;
};

struct LB_EPDPanelDef {
  const char   *id;
  const char   *name;
  LB_EPDDriver  driver;
  uint16_t      width;        // native, in pixels: the SOURCE axis (the short side
                              // on small panels; the long one on 7.5")
  uint16_t      height;       // native: the GATE axis
  uint8_t       rotation;     // how the canvas starts: the way the panel is held
  bool          mirrorGate;   // default scan runs the long axis backwards
  uint16_t      fullMs;       // measured; for timeouts and printInfo()
  uint16_t      partialMs;    // 0 = the panel has no partial refresh (colour panels)
  uint8_t       inks;         // 2 = black and white; 3 = black, white and red
  const LB_EPDWiring *wiring; // fixed wiring of a board with the panel on it
};

/* LilyGO T5 2.13" (SKU H239, T5 V2.3.1 / V2.4). Pins as LilyGO's own examples
 * construct the panel (Xinyuan-LilyGO/LilyGo-T5-Epaper-Series,
 * examples/DeepSleep: CS 5, DC 17, RST 16, BUSY 4) on the VSPI pins 18 / 23.
 * BUSY is 4 here, not the 19 of our QC rig. */
// VSPI is the classic ESP32's second user SPI bus. The name does not exist on
// the ESP32-S3, where the equivalent is FSPI - the T5 is a classic-ESP32
// board, but the library still has to compile for either.
#ifdef VSPI
  #define LB_EPD_SPI_HOST VSPI
#else
  #define LB_EPD_SPI_HOST FSPI
#endif

static const LB_EPDWiring LB_WIRING_LILYGO_T5 = {5, 17, 16, 4, 18, 23, LB_EPD_SPI_HOST};

static const LB_EPDPanelDef LB_EPD_PANELS[] = {
  /* DEPG0213BN (DKE), SSD1680, 122 x 250. Measured on a T5 (2026-09-24),
   * room temperature, internal temperature sensor: full 4189 ms, partial
   * 742 ms (one of ~60 was 863), steady to the millisecond over runs. The
   * refresh time follows the temperature - the controller picks the
   * waveform by it - so expect it to move with the weather.
   * With the scan order set explicitly (LB_EPD.cpp, setWindow) the long axis
   * needs no mirroring; the QC rig's "mirrored long axis" was a property of
   * the power-on defaults. Rotation 1 = landscape, text along the long side. */
  {"lilygo_t5_213", "LilyGO T5 2.13 inch", LB_EPD_SSD1680, 122, 250, 1,
   false, 4189, 742, 2, &LB_WIRING_LILYGO_T5},

  /* HINK-E0213A22-A0, 2.13" black and white, bare panel on an adapter,
   * wired to a classic ESP32 on the T5's pins. The T5's settings drive it
   * unchanged - same 122 x 250 SSD1680 geometry, same orientation.
   * Measured 2026-09-29, room temperature: full 3703 ms, partial 661 ms
   * (five in a row, all 661). */
  {"hink_e0213a22", "HINK-E0213A22 2.13 inch", LB_EPD_SSD1680, 122, 250, 1,
   false, 3703, 661, 2, &LB_WIRING_LILYGO_T5},

  /* !! UNVERIFIED - the HINK-E075A07 entry has not drawn a pixel: that panel
   * is believed dead. Kept so the next one can be tried in one line; not in
   * the README until it works.
   *
   * HINK-E075A07-A0, 7.5" black / white / red, 880 x 528, SSD1677 (the same
   * glass and controller as GoodDisplay GDEH075Z90 and Waveshare's 7.5" HD
   * (B)). A bare panel on an HINK adapter, wired by the user to a classic
   * ESP32 on the T5's pins. Sequence from Waveshare's EPD_7in5b_HD.c. No
   * partial refresh: every update runs the full three-colour waveform.
   * Landscape at rotation 0 - the source axis is the long one here. */
  {"hink_e075a07", "HINK-E075A07 7.5 inch BWR", LB_EPD_SSD1677, 880, 528, 0,
   false, 20000, 0, 3, &LB_WIRING_LILYGO_T5},

  /* HED-EPD0750-2BWR (FPC-5904 REV B), 7.5" black / white / red — 640 x 384 on
   * a UC8159, the generation GoodDisplay sold as GDEW075Z09 and Waveshare as
   * 7.5" (B) V1. NOT the 800 x 480 UC8179 its size suggests: the UC8179
   * sequence was accepted and did nothing (refresh "done" in 36 ms). Found by
   * trying drivers until one refreshed for 16.9 s. Sequence from Waveshare's
   * EPD_7in5bc.c; pixels are 4 bits each, 0 black, 3 white, 4 red.
   * Verified 2026-09-29 on a classic ESP32: colours and orientation correct,
   * BUSY 8375 ms for a full refresh. */
  {"hed_epd0750_bwr", "HED-EPD0750 7.5 inch BWR", LB_EPD_UC8159, 640, 384, 0,
   false, 8375, 0, 3, &LB_WIRING_LILYGO_T5},

  /* WFT0420CZ15 (ribbon marking), 4.2" black and white, 400 x 300, UC8176 —
   * the panel GoodDisplay sells as GDEW042T2 and Waveshare as 4.2" (V1).
   * Waveform from the panel's OTP (panel setting 0x1F), not host LUTs.
   * Verified 2026-09-29 on a classic ESP32: correct picture and orientation,
   * BUSY 3913 ms for a full refresh. No partial refresh yet (it needs host LUTs). */
  {"wft0420cz15", "WFT0420CZ15 4.2 inch", LB_EPD_UC8176, 400, 300, 0,
   false, 3913, 0, 2, &LB_WIRING_LILYGO_T5},
};

// ── Boards with a panel built in ─────────────────────────────
// Not sold by Lonely Binary; named after the board, so they cannot be
// confused with a product.
#define LB_LILYGO_T5_213 (&LB_EPD_PANELS[0])  // LilyGO T5 2.13", SKU H239

// ── Bare panels ──────────────────────────────────────────────
// Named after the panel's own part number. The wiring is the T5's pin set
// (CS 5, DC 17, RST 16, BUSY 4, SCK 18, MOSI 23); use setWiring() for others.
#define LB_HINK_E0213A22 (&LB_EPD_PANELS[1]) // 2.13" 250x122, black/white
#define LB_HINK_E075A07 (&LB_EPD_PANELS[2])  // 7.5" 880x528, black/white/red
#define LB_HED_EPD0750  (&LB_EPD_PANELS[3])  // 7.5" 640x384, black/white/red (UC8159)
#define LB_WFT0420CZ15  (&LB_EPD_PANELS[4])  // 4.2" 400x300, black/white  // 7.5" 800x480, black/white/red
