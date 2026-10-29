# LB_437_BWR — 4.37" black / white / red e-paper

```cpp
LB_EPaper display(LB_437_BWR);
```

## Specification

| | |
|---|---|
| Size | 4.37 inch |
| Resolution | 176 × 480 native (source × gate); 480 × 176 landscape in the library |
| Inks | black, white, red (`LB_BLACK`, `LB_WHITE`, `LB_RED`) |
| Controller | UC81xx family (identified by behaviour, not by part number) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | low = busy |
| Frame memory | two 1-bit planes, 22 bytes × 480 rows = 10 560 bytes each, 21 120 bytes in all |
| Partial refresh | none (colour panel: the whole waveform always runs) |
| Init | none — no register is written; the panel's OTP configuration is correct |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene. Sketch: `LibBench`.

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 13 748 ms | 14 039 ms |
| Full black | 13 748–13 750 ms | 14 039–14 040 ms |
| Full red | 13 750 ms | 14 040–14 041 ms |
| Text, black on white | 13 749–13 750 ms | 14 039–14 041 ms |
| Black / white / red checkerboard | 13 750 ms | 14 040 ms |
| Mixed page (text, black block, red block) | 13 750 ms | 14 040–14 041 ms |

- **The refresh takes the same ~13.75 s whatever is on the screen.** Single
  colour and three colours cost the same: one fixed waveform, no shortcut.
- `flush()` adds about **290 ms** to the BUSY time: reset, sending 21 120 bytes
  at 4 MHz, power on, power off.
- Repeatability is ±2 ms over 18 refreshes.

Not measured yet: temperature dependence, supply current, ghosting
(judge by eye after a black-to-white change), and refresh time at other SPI clocks.

## Wiring on the ESP32-S3 (as tested)

| Signal | GPIO |
|---|---|
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 2 |
| RST | 42 |
| BUSY | 41 |

The library's built-in pin set is the classic ESP32's (CS 5, DC 17, RST 16,
BUSY 4, SCK 18, MOSI 23). On an ESP32-S3 pass your own with `setWiring()`;
see `LibCheck` for a complete sketch.

## Orientation

The panel's short side carries the FPC. The library sets rotation 3, which
reads upright with the FPC on the right (rotation 1 reads upright with the FPC
on the left). Rotation can be changed with `setRotation()` like on any
Lonely Binary display.

## Notes on this panel

- The source axis is backwards in hardware: every row goes out last byte first
  and bit-reversed. The library does this; with raw drivers the symptom is a
  mirrored page with every element present.
- Do not write `0x50` (CDI) or `0x61` (TRES): they hang the refresh or the
  power-on. `0x00` (PSR) flips the picture. Leave the OTP defaults alone.
- No datasheet or vendor sample code exists for this panel; everything above was
  found on hardware. A yellow-ink unit of the same glass was measured at
  ~25 s full refresh — the red variant is faster.
