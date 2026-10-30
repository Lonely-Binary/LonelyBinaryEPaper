# LB_154_BWR — 1.54" black / white / red e-paper

```cpp
LB_EPaper display(LB_154_BWR);
```

## Specification

| | |
|---|---|
| Size | 1.54 inch (ribbon HINK-E0154A34) |
| Resolution | 152 × 152 (square) |
| Inks | black, white, red (`LB_BLACK`, `LB_WHITE`, `LB_RED`) |
| Controller | SSD1682 family (the command set of the 200 × 200 three-colour panels, 152-gate scan) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | high = busy |
| Frame memory | two 1-bit planes, 19 bytes × 152 rows = 2 888 bytes each, 5 776 bytes in all |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene. Sketch: `LibBench`.

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 16 154 ms | 16 439–16 440 ms |
| Full black | 16 154–16 155 ms | 16 439–16 441 ms |
| Full red | 16 156–16 157 ms | 16 441–16 442 ms |
| Text, black on white | 16 155–16 156 ms | 16 440–16 442 ms |
| Black / white / red checkerboard | 16 156 ms | 16 441–16 442 ms |
| Mixed page (text, black block, red block) | 16 156–16 157 ms | 16 441–16 442 ms |

- **The refresh takes the same ~16.16 s whatever is on the screen.** One colour
  and three colours cost the same: one fixed waveform, no shortcut.
- `flush()` adds about **285 ms** to the BUSY time: reset, sending 5 776 bytes,
  loading the registers, power on and off.
- Repeatability is ±2 ms over 18 refreshes.
- QC measured 13.3 s on a classic ESP32 with GxEPD2; on this ESP32-S3 setup the
  same panel takes 16.2 s. The waveform is chosen by the controller from its
  temperature sensor, so the room temperature on the day moves it.

Not measured yet: temperature dependence, supply current, ghosting, refresh
time at other SPI clocks.

## Wiring on the ESP32-S3 (the library's default for this panel)

| Signal | GPIO |
|---|---|
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 2 |
| RST | 42 |
| BUSY | 41 |

Other pins: `setWiring()`. Sketches: `LibCheck` and `LibBench`.

## Orientation

The library sets rotation 1, a quarter turn right of the panel's native
orientation (rotation 0, upright with the FPC at the bottom). Verified on the
panel: the asymmetric `F` test page, black, red and the frame come out upright
and complete. `setRotation()` turns it any way you like.

## Notes on this panel

- No datasheet. Same commands as the 200 × 200 SSD1682 three-colour panels; the
  one thing that must change is the gate count (152 → driver output control
  `0x97`), or the picture is squashed and the bottom strip never refreshes.
- Border waveform `0x3C 0x05`; red plane in RAM `0x26` (1 = red).
- No partial refresh. Run a black / white / red clearing cycle now and then if a
  ghost builds up.
