#pragma once
/*
 * LB_EPDPanels.h - the e-paper panels this library drives.
 *
 * One entry per panel. Everything that differs between two panels on the same
 * controller is here; the refresh sequences themselves are in LB_EPD.cpp.
 *
 * (Hand-written for now, while there is one panel. It moves to a panels.yaml
 * and a generator, as in Lonely Binary Display, when the second one arrives.)
 */
#include <Arduino.h>  // VSPI / HSPI

enum LB_EPDDriver : uint8_t {
  LB_EPD_SSD1680,   // Solomon SSD1680 / SSD168x black-and-white: BUSY high = busy
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
  uint16_t      width;        // native, in pixels: the SOURCE axis (short side)
  uint16_t      height;       // native: the GATE axis (long side)
  uint8_t       rotation;     // how the canvas starts: the way the panel is held
  bool          mirrorGate;   // default scan runs the long axis backwards
  uint16_t      fullMs;       // measured; for timeouts and printInfo()
  uint16_t      partialMs;    // 0 = the panel has no partial refresh (colour panels)
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
   false, 4189, 742, &LB_WIRING_LILYGO_T5},
};

// ── Boards with a panel built in ─────────────────────────────
// Not sold by Lonely Binary; named after the board, so they cannot be
// confused with a product.
#define LB_LILYGO_T5_213 (&LB_EPD_PANELS[0])  // LilyGO T5 2.13", SKU H239
