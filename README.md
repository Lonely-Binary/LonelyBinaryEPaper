# Lonely Binary EPaper

E-paper with the same drawing API as every Lonely Binary display.

```cpp
LB_EPaper display(LB_LILYGO_T5_213);   // ← the only line that changes
```

That constant resolves the controller, resolution, orientation, refresh
timings and pins. Drawing is **Lonely Binary GFX** — `fillRect`, `drawString`,
`drawJpg`, … — the same calls as on a Lonely Binary TFT or VGA display, so a
function written against `LB_Canvas &` runs on all three.

The panel driver is the library's own, written from the panel vendor's sample
code and from drivers verified on real panels. It does not use GxEPD2, which
is GPL-3.0; this library is MIT.

> **Writing your code with an AI?** Give it [`llms.txt`](llms.txt) first.

---

## Supported panels

Lonely Binary's own panels, smallest first.

| Constant | Panel | Resolution | Controller | Full / partial refresh |
|---|---|---|---|---|
| `LB_154_BW` | 1.54" black and white, Lonely Binary panel — [specification](docs/LB_154_BW.md) | 152 × 152 | SSD1682 | 3.5 s / — |
| `LB_154_BW_1` | 1.54" black and white, Lonely Binary panel — [specification](docs/LB_154_BW_1.md) | 200 × 200 | JD79653A | 3.6 s / — |
| `LB_154_BWR` | 1.54" **black, white and red**, Lonely Binary panel — [specification](docs/LB_154_BWR.md) | 152 × 152 | SSD1682 | 16.4 s / — |
| `LB_154_BWY` | 1.54" **black, white and yellow**, Lonely Binary panel — [specification](docs/LB_154_BWY.md) | 152 × 152 | SSD1682 | 23.1 s / — |
| `LB_213_BWR` | 2.13" **black, white and red**, Lonely Binary panel — [specification](docs/LB_213_BWR.md) | 250 × 122 | SSD1680 | 14.3 s / — |
| `LB_266_BWR` | 2.66" **black, white and red**, Lonely Binary panel — [specification](docs/LB_266_BWR.md) | 296 × 152 | SSD1680 | 15.2 s / — |
| `LB_420_BWRY` | 4.2" **black, white, red and yellow**, Lonely Binary panel — [specification](docs/LB_420_BWRY.md) | 400 × 300 | UC81xx | 23.5 s / — |
| `LB_437_BWR` | 4.37" **black, white and red**, Lonely Binary panel — [specification](docs/LB_437_BWR.md) | 480 × 176 | UC81xx | 14.0 s / — |
| `LB_583_BWR` | 5.83" **black, white and red**, Lonely Binary panel — [specification](docs/LB_583_BWR.md) | 648 × 480 | UC8179 | 12.3 s / — |
| `LB_583_BWR_1` | 5.83" **black, white and red**, the second Lonely Binary 5.83" model — [specification](docs/LB_583_BWR_1.md) | 648 × 480 | UC8179 | 20.1 s / — |
| `LB_583_BWY` | 5.83" **black, white and yellow**, same panel as `LB_583_BWR_1` — [specification](docs/LB_583_BWR_1.md) | 648 × 480 | UC8179 | 17.1 s / — |

Three-ink panels take `LB_RED` (or `LB_YELLOW` on a yellow panel) like any other colour: `display.setTextColor(LB_RED)`. The four-ink `LB_420_BWRY` takes both `LB_RED` and `LB_YELLOW`.

### Third-party panels

Not sold by Lonely Binary; they are here because the library was first proven
on them.

| Constant | Panel | Resolution | Controller | Full / partial refresh |
|---|---|---|---|---|
| `LB_LILYGO_T5_213` | LilyGO T5 2.13" (SKU H239), DEPG0213BN | 250 × 122 | SSD1680 | 4.2 s / 0.74 s |
| `LB_HINK_E0213A22` | HINK-E0213A22-A0 2.13", bare panel | 250 × 122 | SSD1680 | 3.7 s / 0.66 s |
| `LB_WFT0420CZ15` | 4.2" black and white (ribbon WFT0420CZ15; GDEW042T2), bare panel | 400 × 300 | UC8176 | 3.9 s / — |
| `LB_HED_EPD0750` | 7.5" **black, white and red** (HED-EPD0750-2BWR; GDEW075Z09 generation), bare panel | 640 × 384 | UC8159 | 8.4 s / — |

The frame is kept as two 1-bit planes, so a 7.5" needs 2 × 30 KB — it fits a
classic ESP32 without PSRAM.

The T5 is a board with the panel built in, so its constant carries its pins
too. A bare panel's constant uses the same pins
(CS 5, DC 17, RST 16, BUSY 4, SCK 18, MOSI 23); wire it differently and pass
your pins with `setWiring()`. Lonely Binary's own panels use the ESP32-S3 pins
given in each specification.

---

## Arduino

```cpp
#include <LonelyBinaryEPaper.h>

LB_EPaper display(LB_LILYGO_T5_213);

void setup() {
  display.begin();

  display.fillScreen(LB_WHITE);
  display.setTextColor(LB_BLACK);
  display.setTextSize(2);
  display.drawString("Hello", 10, 10);
  display.flush();                    // nothing appears before this
}

void loop() {}
```

### Examples

**File ▸ Examples ▸ Lonely Binary EPaper**, in order — each one builds on the
ones before it:

| | |
|---|---|
| `00_Hello` | The smallest sketch. Run it first. |
| `01_PinCheck` | Black, white and a geometry page, each refresh timed and judged. Run it on new hardware. |
| `02_Text` | Two fonts, scaling, measured alignment, UTF-8. |
| `03_Shapes` | `draw*` and `fill*`, and grey on a screen that has none. |
| `04_Rotation` | Portrait and landscape; `width()` and `height()` follow. |
| `05_Clock` | Partial refresh: HH:MM:SS where the panel has it, HH:MM where it does not. |
| `06_Icons` | Line icons drawn with shapes, and a bitmap icon. |
| `07_Chart` | A chart that grows one point per partial refresh. |
| `08_Dashboard` | Four readings with icons, laid out from the screen size. |
| `09_DeepSleep` | Draw, then deep-sleep the ESP32; the picture stays with no power. |
| `10_DeepClean` | Wipe a ghost: every ink, full screen, several rounds, ending on white. Run it when an old picture shows through. |
| `11_MinuteClock` | A colour-panel desk clock: one full refresh a minute, time on a red card, date, day progress, chip temperature. Made for the 2.66" BWR; one optional pin switches the panel's power. |

### Three things that are different from a TFT

**Nothing appears until `flush()`.** Everything is drawn into a framebuffer in
RAM (4 KB for the 2.13"), and `flush()` sends it to the panel. Draw the whole
screen, then flush once.

**`flush()` blocks.** A full refresh takes about 4 seconds on the T5, and the whole
screen flashes black and white while it runs. That is normal for e-paper. The
time moves with the temperature: the panel picks its waveform by it.

**Partial refresh is faster and does not flash** — 0.74 s on the T5:

```cpp
display.flush(LB_FLUSH_PARTIAL);
```

Only what changed moves. Two rules:

- The first `flush()` is always full, whatever you ask for: a partial refresh
  needs the previous frame on the panel to compare against.
  `lastWasPartial()` says which one ran.
- Partial refreshes leave a faint ghost that builds up. Do a full refresh now
  and then — every 30 updates or so — to wipe it.

After `sleep()`, the next refresh is full again.

### Ghosting on colour panels — refresh regularly

An e-paper picture is made of charged ink particles. Leave the same picture
up for a long time — days or weeks — and the particles settle where they are:
the next refresh no longer moves all of them, and the old picture stays
faintly visible under the new one. This is a **ghost** (image retention).

**Colour panels (black / white / red or yellow) show it most.** The red and
yellow particles are slower than the black ones, so a large solid block of red
or yellow left up for days is the typical case: it comes back as a pink or
grey shadow on white. Cold makes it worse. A ghost is not a broken panel, and
it can almost always be cleared.

To keep it away:

- **Refresh at least once a day**, even when nothing has changed — redraw the
  same screen if you have to.
- Do not leave a large solid block of red or yellow up for days; move it,
  shrink it, or outline it instead of filling it.
- Clear the screen to white before putting the device away.
- Run a clearing cycle now and then — once a day, or every few hundred
  refreshes.

**To remove a ghost, run `10_DeepClean`.** It fills the whole screen with each
ink in turn (black → white → red → white), three rounds, and ends on white. A
ghost left for weeks may need two runs. On a colour panel every refresh runs
the whole colour waveform, so this takes a few minutes (about 3 on the 2.13"
red panel); do not unplug it halfway. Its `deepClean()` function can be copied
into your own sketch as it is.

### Power

The panel is put into deep sleep after every refresh, so it never stays
powered — that is what damages e-paper over time. The picture stays on the
screen with no power at all, so the ESP32 can deep-sleep too.

### API

| | |
|---|---|
| `begin()` | Allocate the framebuffer, start SPI, clear to white, set the panel's rotation. Does not touch the screen. |
| Drawing | `fillScreen`, `fillRect`, `drawLine`, `drawCircle`, `drawString`, `drawJpg`, … — the Lonely Binary GFX API. Use `LB_BLACK` and `LB_WHITE`, plus `LB_RED` (`LB_YELLOW` on a yellow panel) on a three-ink panel, and both on the four-ink `LB_420_BWRY`; any other colour is mapped to the nearest ink the panel has. |
| `flush()` / `flush(LB_FLUSH_PARTIAL)` | Refresh. Blocks until the panel is done. |
| `lastRefreshMs()` / `lastWasPartial()` | How the last refresh went. A time far below normal is as much a failure as `-1` (timeout). |
| `sleep()` | Forget the previous frame; the next refresh is full. |
| `setRotation(0..3)` | Rotated in software. |
| `width()` / `height()` | Current size, rotation included. |
| `setWiring(pins)` | Your own pins, before `begin()`. Every example lists its pins as `#define PIN_...` at the top and calls this for you. Start from the panel's: `LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;` |
| `printInfo()` | Panel, pins and the last refresh, on Serial. |

---

## License

MIT. Brought to you by Lonely Binary — this library exists because of the
wonderful customers who buy our products and keep supporting our open-source
work. Thank you!
