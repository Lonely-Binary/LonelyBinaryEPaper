# LB_420_BWRY — 4.2" black / white / red / yellow e-paper

```cpp
LB_EPaper display(LB_420_BWRY);
```

## Specification

| | |
|---|---|
| Size | 4.2 inch (DKE / GYSDISPLAY DEPG0420RYF668F38; label NF420N005FS915, FPC 15-22243-M0061-42_A1) |
| Resolution | 400 × 300, landscape |
| Inks | black, white, red, yellow (`LB_BLACK`, `LB_WHITE`, `LB_RED`, `LB_YELLOW`) |
| Controller | UC81xx style (factory sample code; no part number) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | low = busy |
| Panel RAM | one 2-bit-a-pixel frame, command `0x10`, 30 000 bytes: 0 black, 1 white, 2 yellow, 3 red |
| Frame memory in the library | two 1-bit planes, 15 000 bytes each (30 000 in all); packed to 2 bits a row at a time during `flush()` |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 refreshes per
scene, 2026-11-06 (second run, 16 refreshes; a first run of 16 agreed to within
10 ms).

| Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|
| Full white | 23 476–23 482 ms | 24 065–24 071 ms |
| Full black | 23 477–23 482 ms | 24 067–24 072 ms |
| Full red | 23 476–23 481 ms | 24 065–24 070 ms |
| Full yellow | 23 477–23 481 ms | 24 067–24 071 ms |
| Mixed page (text, all four inks, fine lines) | 23 478–23 486 ms | 24 067–24 075 ms |

- **The refresh takes the same ~23.5 s whatever is on the screen.** One fixed
  four-colour waveform, no shortcut for a single colour.
- `flush()` adds about **590 ms** to the BUSY time: reset, init, packing and
  sending 30 000 bytes at 4 MHz, power on (~134 ms), power off.
- Repeatability is ±5 ms over 16 refreshes in a row.
- It moves with temperature: the controller picks the waveform by it. A refresh
  of the same page half an hour later, after the panel had been handled, took
  17 362 ms (flush 17 949 ms), and QC measured 19.8 s on a classic ESP32 on
  another day. Plan for anything from about 17 to 24 s at room temperature.

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

Rotation 0: landscape, upright with the FPC tab at the bottom. Verified on the
panel (2026-11-06, ESP32-S3) with an asymmetric `F`, corner blocks of three
different inks and a block of each ink: nothing mirrored, nothing missing.

## Notes on this panel

- The init sequence is the factory sample's (`DEPG0420RY683FxX.c`), as proven
  by the direct driver in the QC repository. Its register values differ from
  the 2.13" and 2.9" four-ink panels; do not copy theirs.
- **`0x12` (display refresh) takes a parameter byte, `0x00`.** Without it the
  command is accepted, BUSY never moves and the "refresh" returns in about
  200 ms with nothing drawn.
- After `0x04` (power on) BUSY is low for about 134 ms — a quick check that the
  panel is powered and listening.
- Drawing uses the four inks directly; any other colour is mapped to the
  nearest of the four (orange, for example, becomes red or yellow).
- On an ESP32-S3, do not touch GPIO 19 or 20 in a test sketch: they are the
  USB pins, and reconfiguring them drops the board off USB.
