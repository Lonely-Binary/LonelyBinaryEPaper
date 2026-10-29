# LB_213_BWR — 2.13" black / white / red e-paper

```cpp
LB_EPaper display(LB_213_BWR);
```

## Specification

| | |
|---|---|
| Size | 2.13 inch |
| Resolution | 122 × 250 native; 250 × 122 landscape in the library (rotation 1) |
| Inks | black, white, red (`LB_BLACK`, `LB_WHITE`, `LB_RED`) |
| Controller | SSD1680, three-colour (same glass as GoodDisplay GDEY0213Z98) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | high = busy |
| Frame memory | two 1-bit planes, 16 bytes × 250 rows = 4 000 bytes each, 8 000 bytes in all |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene. Sketch: `LibBench`.

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 13 978–13 979 ms | 14 268–14 269 ms |
| Full black | 13 977–13 979 ms | 14 267–14 269 ms |
| Full red | 13 977–13 978 ms | 14 267–14 268 ms |
| Text, black on white | 13 977–13 978 ms | 14 267–14 268 ms |
| Black / white / red checkerboard | 13 977 ms | 14 267 ms |
| Mixed page (text, black block, red block) | 13 977–13 978 ms | 14 267–14 268 ms |

- **The refresh takes the same ~13.98 s whatever is on the screen.** One colour
  and three colours cost the same: one fixed waveform, no shortcut.
- `flush()` adds about **290 ms** to the BUSY time: reset, sending 8 000 bytes,
  loading the registers, power on and off.
- Repeatability is ±1 ms over 18 refreshes.

Not measured yet: temperature dependence (the controller picks the waveform by
temperature), supply current, ghosting, refresh time at other SPI clocks.

## Wiring on the ESP32-S3 (the library's default for this panel)

| Signal | GPIO |
|---|---|
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 2 |
| RST | 42 |
| BUSY | 41 |

Other pins: `setWiring()`. Sketches: `LibCheck` (one page, prints the refresh
time) and `LibBench` (this table).

## Orientation

Rotation 1 (landscape). Verified on the panel: the asymmetric `F` test page,
black, red and the white frame all come out upright, the full frame is
addressed, nothing is cropped or mirrored.

## Notes on this panel

- Same controller and geometry as the black-and-white 2.13" panels; only the
  colour set-up differs: border waveform `0x3C 0x05`, display update control
  `0x21 0x00 0x80`, red plane in RAM `0x26` (1 = red).
- No partial refresh. Do a black / white / red clearing cycle now and then if a
  ghost builds up.
