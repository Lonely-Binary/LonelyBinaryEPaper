# LB_583_BWR_1 / LB_583_BWY — the second 5.83" model, red or yellow ink

```cpp
LB_EPaper display(LB_583_BWR_1);   // red ink
LB_EPaper display(LB_583_BWY);     // the same panel with yellow ink
```

**Not the same panel as `LB_583_BWR`.** Both are 5.83" 648 × 480 on a UC8179-class
controller, but this model needs two things the first does not. With the wrong
constant it either never powers up or draws shifted 16 columns to the left.

## Specification

| | |
|---|---|
| Size | 5.83 inch |
| Resolution | 648 × 480, landscape (rotation 0) |
| Inks | `LB_583_BWR_1`: black, white, red (`LB_RED`). `LB_583_BWY`: black, white, yellow (`LB_YELLOW`) |
| Controller | UC8179 class: BUSY low = busy; a bare power-on `0x04` takes ~142 ms |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| Frame memory | two 1-bit planes, 81 bytes × 480 rows = 38 880 bytes each; sent as 664-wide rows (2 bytes of padding on the left) |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## The two quirks (both handled by the library)

1. **Warm-up power-on.** From cold, a power-on with the full UC8179 power
   settings never finishes: BUSY stays low. A power-on with the controller's
   defaults first fixes it, five times out of five on two units. The library
   does that warm-up before every refresh (`warmup` in the panel table). It adds
   about 0.5 s.
2. **16 dead source columns.** The visible area is source columns 16–663 of
   664. Measured with a staircase test, 8 columns a step: the two leftmost and
   two rightmost steps fall outside the glass. The library tells the controller
   664 columns and pads every row with 16 white columns on the left
   (`sourceOffset` in the panel table), so the canvas is still 648 × 480 and
   x = 0 is the first visible column.

## Telling the two 5.83" models apart

| | `LB_583_BWR` | `LB_583_BWR_1` / `LB_583_BWY` |
|---|---|---|
| Panel marking | HINK-E0583A01-A0, label HE583A02RE | not recorded yet |
| Bare power-on `0x04` | ~55 ms | ~142 ms |
| Full refresh | 11.5–12.3 s | red ~18.8 s, yellow ~15.8 s |
| Symptom with the other constant | — | `LB_583_BWR` on this panel: refresh never starts from cold; when it does, the picture is 16 columns too far left with a noise strip on the right |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 2026-11-06.

| Panel | Scene | BUSY (refresh) | Whole `flush()` |
|---|---|---|---|
| `LB_583_BWR_1` (red) | dashboard demo page, 1 run | 18 826 ms | 20 110 ms |
| `LB_583_BWY` (yellow) | demo page and raw test patterns, 4 runs | 15 799–15 812 ms | 17 085 ms (demo) |

**Not yet measured:** the full 13-refresh benchmark (white / black / colour /
page) on the red unit. The figures above come from single pages. Expect the
time to move with temperature.

## Is it a four-colour panel?

No: it has three inks. Tested on the yellow unit:

- `0x12` starts a refresh **without** a parameter byte. That is UC8179
  behaviour; the four-ink controllers here (`LB_420_BWRY`, JD79661) ignore
  `0x12` unless it is followed by `0x00`.
- The four-ink sequence (2 bits a pixel in `0x10`, the DEPG0420RY factory
  registers) was rejected. BUSY went busy for 69 ms and the screen did not
  change.
- With the three-ink driver the colour plane is a clean, even yellow (red on
  the red unit). A reddish tint seen in the panel's old content is yellow ink
  caught mid-waveform.

## Wiring on the ESP32-S3 (the library's default for this panel)

| Signal | GPIO |
|---|---|
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 2 |
| RST | 42 |
| BUSY | 41 |

Adapter: BS low (4-wire SPI). Tested with RESE at 3 Ω (the warm-up quirk was
found and fixed there). Other pins: `setWiring()`.

## Orientation

Rotation 0, landscape. Verified on both inks with a dashboard page: title bar,
text, blocks and a bar chart upright and not mirrored; all four corner blocks
and the outer frame visible.
