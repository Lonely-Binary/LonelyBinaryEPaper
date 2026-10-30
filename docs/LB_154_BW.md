# LB_154_BW — 1.54" black and white e-paper

```cpp
LB_EPaper display(LB_154_BW);
```

## Specification

| | |
|---|---|
| Size | 1.54 inch (DKE DEPG0154BNF653F10CPHO) |
| Resolution | 200 × 200 (square) |
| Inks | black, white (`LB_BLACK`, `LB_WHITE`) |
| Controller | Fitipower JD79653A (UC81xx-style command set) |
| Interface | 4-wire SPI + DC, RST, BUSY. 3.3 V — never 5 V |
| BUSY | low = busy |
| Frame memory | one 1-bit plane, 25 bytes × 200 rows = 5 000 bytes |
| Partial refresh | none (see below) |

## Measured refresh time

ESP32-S3 (Arduino-ESP32), room temperature, SPI 4 MHz.

| Step | BUSY |
|---|---|
| Power on | ~160 ms |
| Full refresh | 3 577–3 582 ms |
| Power off | ~60 ms |

The waveform comes from the panel's OTP and is picked by the controller's
temperature sensor, so the time moves with the room temperature.

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

The library sets rotation 1, a quarter turn right of the native orientation,
for the panel held with the FPC on the right. Verified on the panel: an
asymmetric test page, the frame and the corner marks come out upright and
complete. `setRotation()` turns it any way you like.

## Notes on this panel

- The controller needs its full init before power-on: with a short UC81xx
  init, power-on never completes.
- Panel setting `0x00 0xDF 0x0E` takes the waveform from the OTP. `0xFF`
  (waveform from registers) without loading tables runs an empty waveform:
  BUSY for ~77 ms and nothing changes.
- `0x13` is the frame shown; `0x10` is the old frame, sent white.
- **No partial refresh.** The OTP holds no partial waveform. Register-loaded
  partial waveforms were tried (a published table set, then the same with
  twice the drive): the update took 318–540 ms, but every pixel that went
  black → white stayed light grey, a visible ghost of each change. Until a
  waveform made for this glass is available, `flush(LB_FLUSH_PARTIAL)` runs a
  full refresh.
- One unit's BUSY read inverted and never released; a second unit behaved as
  above. If `flush()` takes ~4× the time above, check BUSY first.
