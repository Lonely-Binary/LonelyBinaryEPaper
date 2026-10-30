# LB_154_BWY — 1.54" black / white / yellow e-paper

```cpp
LB_EPaper display(LB_154_BWY);
```

## Specification

| | |
|---|---|
| Size | 1.54 inch (DKE DEPG0154YNS7xxF10P-HO) |
| Resolution | 152 × 152 (square) |
| Inks | black, white, yellow (`LB_BLACK`, `LB_WHITE`, `LB_YELLOW`) |
| Controller | SSD16xx family; driven like `LB_154_BWR` (SSD1682 path, 152-gate scan) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | high = busy |
| Frame memory | two 1-bit planes, 2 888 bytes each; colour plane RAM `0x26`, 1 = yellow |
| Partial refresh | none (colour panel: the whole waveform always runs) |

## Refresh time

23 051 ms (`lastRefreshMs()`, ESP32-S3, room temperature, one run on a test page of text, black and yellow blocks). QC's probe saw about 23 s. The controller picks the waveform by temperature, so it moves with the weather. Not yet repeated over several runs or other content.

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

Rotation 1, as `LB_154_BWR`. Verified on the panel (2026-10-30, ESP32-S3):
yellow shows as yellow, and an asymmetric `F` and the corner blocks come out
upright and complete. No RAM column offset is needed.

## Notes

- Same commands as `LB_154_BWR`; only the third ink differs.
- Run a black / white / yellow clearing cycle now and then if a ghost builds up.
