# LB_583_BWR — 5.83" black / white / red e-paper

```cpp
LB_EPaper display(LB_583_BWR);
```

## Specification

| | |
|---|---|
| Size | 5.83 inch (HINK-E0583A01-A0; label HE583A02RE) |
| Resolution | 648 × 480, landscape (rotation 0) |
| Inks | black, white, red (`LB_BLACK`, `LB_WHITE`, `LB_RED`) |
| Controller | UC8179 class (the GDEW0583Z83 generation), identified by behaviour: BUSY low = busy, low ~55 ms after power-on `0x04` |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | low = busy |
| Frame memory | two 1-bit planes, 81 bytes × 480 rows = 38 880 bytes each, 77 760 bytes in all; `0x10` black/white (1 = white), `0x13` red (1 = red) |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene, 2026-11-06.

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 11 539–11 543 ms | 12 309–12 314 ms |
| Full black | 11 539–11 540 ms | 12 311–12 312 ms |
| Full red | 11 537–11 543 ms | 12 308–12 315 ms |
| Mixed page (text, black and red blocks, frame) | 11 536–11 538 ms | 12 307–12 310 ms |

- **The refresh takes the same ~11.5 s whatever is on the screen.** One fixed
  three-colour waveform, no shortcut.
- `flush()` adds about **770 ms** to the BUSY time: reset, init, sending
  77 760 bytes at 4 MHz, power on, power off.
- Repeatability is ±4 ms over 13 refreshes in a row.
- The controller picks the waveform by temperature, so expect it to move with
  the weather.
- **A second 5.83" unit** (same probe signature: BUSY low ~55 ms after power-on)
  ran the same driver unchanged, with a slightly longer waveform: BUSY
  12 272–12 277 ms, whole `flush()` 13 044–13 049 ms, over the same 13
  refreshes. Its picture held unchanged for 80 s after the refresh. Plan for
  about 11.5–12.3 s.

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

Rotation 0: landscape. Verified on the panel (2026-11-06, ESP32-S3) with an
asymmetric `F`, black and red blocks, text, the bottom corner blocks and the
outer frame: nothing mirrored, the full 648 × 480 addressed. The top edge (title
bar, top corner blocks) was confirmed on the second unit.

## Notes on this panel

- Same commands as the library's UC8179 path (written for the 7.5" 800 × 480
  generation); only the resolution differs. It drew correctly on the first try.
- The other common 5.83" three-colour panel is 600 × 448 on a UC8159 (4 bits a
  pixel). This one is not that: the UC8179 sequence at 648 × 480 is right.
