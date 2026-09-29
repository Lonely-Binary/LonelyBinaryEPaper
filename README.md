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

| Constant | Panel | Resolution | Controller | Full / partial refresh |
|---|---|---|---|---|
| `LB_LILYGO_T5_213` | LilyGO T5 2.13" (SKU H239), DEPG0213BN | 250 × 122 | SSD1680 | 4.2 s / 0.74 s |
| `LB_HINK_E0213A22` | HINK-E0213A22-A0 2.13", bare panel | 250 × 122 | SSD1680 | 3.7 s / 0.66 s |

The T5 is a board with the panel built in, so its constant carries its pins
too. It is not sold by Lonely Binary; it is here because it is what the
library was first proven on. A bare panel's constant uses the same pins
(CS 5, DC 17, RST 16, BUSY 4, SCK 18, MOSI 23); wire it differently and pass
your pins with `setWiring()`. Lonely Binary's own panels will join this table.

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

### Power

The panel is put into deep sleep after every refresh, so it never stays
powered — that is what damages e-paper over time. The picture stays on the
screen with no power at all, so the ESP32 can deep-sleep too.

### API

| | |
|---|---|
| `begin()` | Allocate the framebuffer, start SPI, clear to white, set the panel's rotation. Does not touch the screen. |
| Drawing | `fillScreen`, `fillRect`, `drawLine`, `drawCircle`, `drawString`, `drawJpg`, … — the Lonely Binary GFX API. Use `LB_BLACK` and `LB_WHITE`; any other colour is mapped to the nearer of the two. |
| `flush()` / `flush(LB_FLUSH_PARTIAL)` | Refresh. Blocks until the panel is done. |
| `lastRefreshMs()` / `lastWasPartial()` | How the last refresh went. A time far below normal is as much a failure as `-1` (timeout). |
| `sleep()` | Forget the previous frame; the next refresh is full. |
| `setRotation(0..3)` | Rotated in software. |
| `width()` / `height()` | Current size, rotation included. |
| `setWiring(pins)` | Your own pins, before `begin()`. Start from the panel's: `LB_EPDWiring pins = *LB_LILYGO_T5_213->wiring;` |
| `printInfo()` | Panel, pins and the last refresh, on Serial. |

---

## License

MIT. Brought to you by Lonely Binary — this library exists because of the
wonderful customers who buy our products and keep supporting our open-source
work. Thank you!
