# LB_266_BWR — 2.66" black / white / red e-paper

```cpp
LB_EPaper display(LB_266_BWR);
```

## Specification

| | |
|---|---|
| Size | 2.66 inch (HINK-E0266A12-FPC-A01; label HE266A27RF) |
| Resolution | 152 × 296 native; 296 × 152 landscape in the library (rotation 1) |
| Inks | black, white, red (`LB_BLACK`, `LB_WHITE`, `LB_RED`) |
| Controller | SSD1680 class, three-colour (identified by behaviour: BUSY high = busy, busy for a moment after SWRESET `0x12`, ignores the UltraChip power-on `0x04`) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | high = busy |
| Frame memory | two 1-bit planes, 19 bytes × 296 rows = 5 624 bytes each, 11 248 bytes in all; red plane RAM `0x26`, 1 = red |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene, 2026-11-06.

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 14 902–14 903 ms | 15 200–15 201 ms |
| Full black | 14 901–14 902 ms | 15 199–15 200 ms |
| Full red | 14 900–14 903 ms | 15 197–15 200 ms |
| Mixed page (text, black and red blocks, frame) | 14 902–14 903 ms | 15 199–15 200 ms |

- **The refresh takes the same ~14.9 s whatever is on the screen.** One fixed
  three-colour waveform, no shortcut.
- `flush()` adds about **300 ms** to the BUSY time: reset, soft reset, sending
  11 248 bytes at 4 MHz.
- Repeatability is ±2 ms over 13 refreshes in a row.
- The controller picks the waveform by temperature, so expect it to move with
  the weather.

Not measured yet: temperature dependence, supply current, ghosting over many
refreshes.

## Wiring on the ESP32-S3 (the library's default for this panel)

| Signal | GPIO |
|---|---|
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 2 |
| RST | 42 |
| BUSY | 41 |

Other pins: `setWiring()`.

## Orientation

Rotation 1, as `LB_213_BWR`: landscape, upright with the FPC on the right.
Verified on the panel (2026-11-06, ESP32-S3) with an asymmetric `F`, corner
blocks and the outer frame: nothing mirrored, nothing cropped.
`setRotation(3)` reads upright with the FPC on the left.

## Notes on this panel

- Same commands and colour path as `LB_213_BWR`; only the geometry differs
  (296 gates instead of 250, 19-byte rows instead of 16). It drew correctly on
  the first try with no panel-specific register.
- No datasheet or vendor sample code was used; the controller was identified
  by probing BUSY after a reset, a soft reset and a power-on command.
