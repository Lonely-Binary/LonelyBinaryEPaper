# LB_154_BW — 1.54" black and white e-paper (152 × 152)

```cpp
LB_EPaper display(LB_154_BW);
```

A 1.54" black-and-white panel, 152 × 152 on the SSD1682 — the glass
`LB_154_BWR` and `LB_154_BWY` use, without the colour ink. The other 1.54"
black-and-white panel, 200 × 200 on the JD79653A, is `LB_154_BW_1`; further
variants are numbered `_2`, `_3`, …

## Specification

| | |
|---|---|
| Size | 1.54 inch (part number: ribbon marking to be added) |
| Resolution | 152 × 152 (square) |
| Inks | black, white |
| Controller | SSD1682 family, same commands as `LB_154_BWR` |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | high = busy |
| Partial refresh | none |

## Measured refresh time

ESP32-S3 (Arduino-ESP32 3.3.10), room temperature, SPI 4 MHz, 3 full refreshes:
BUSY 3 471 ms each, whole `flush()` about 3 755 ms.

Partial refresh was tried: with the partial command sequence enabled, 4
`LB_FLUSH_PARTIAL` flushes each took the same 3 471 ms as a full one, so the
OTP has no partial waveform and the library does not offer it (`partialMs` = 0).

## Wiring on the ESP32-S3 (the library's default)

SCK 12, MOSI 11, CS 10, DC 2, RST 42, BUSY 41. Other pins: `setWiring()`.

## Notes

- Rotation 1, as `LB_154_BWR`.
- Ghosting not yet measured.
