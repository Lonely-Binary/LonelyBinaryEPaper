#pragma once
/*
 * Line icons, drawn with the canvas's own shapes - no image files.
 *
 * Every icon takes a centre (cx, cy) and a size s, the width of its box in
 * pixels, and scales with it: the stroke is s / 12, so a 16-pixel icon and a
 * 64-pixel one look like the same icon. They draw on any LB_Canvas - a TFT,
 * VGA or e-paper - because that is all they use.
 *
 * The one trick: an OUTLINE with a thickness is a filled black shape with a
 * slightly smaller filled white shape on top of it. That works for shapes
 * that are unions of circles and rectangles (the cloud), where drawing an
 * outline directly would leave lines inside the shape.
 *
 * `ink` and `paper` are the two colours; swap them for white-on-black.
 */
#include <LonelyBinaryGFX.h>

namespace Icons {

static inline int16_t stroke(int16_t s) { return s >= 24 ? s / 12 : 1; }

static void ring(LB_Canvas &c, int16_t cx, int16_t cy, int16_t r, int16_t t, lb_color_t ink, lb_color_t paper) {
  c.fillCircle(cx, cy, r, ink);
  if (r - t > 0) c.fillCircle(cx, cy, r - t, paper);
}

static void sun(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s), r = s / 5;
  ring(c, cx, cy, r, t, ink, paper);
  // Eight rays, from just outside the disc to the edge of the box. The
  // diagonals are scaled by 0.707 so all eight end on the same circle.
  const int16_t a = s / 2 - t, b = r + t * 2 + 1;
  const int16_t da = a * 707 / 1000, db = b * 707 / 1000;
  const int16_t rays[8][4] = {{0, -b, 0, -a}, {0, b, 0, a}, {-b, 0, -a, 0}, {b, 0, a, 0},
                              {db, -db, da, -da}, {-db, -db, -da, -da}, {db, db, da, da}, {-db, db, -da, da}};
  for (auto &q : rays)
    for (int16_t k = -(t / 2); k <= t / 2; k++) {
      // thickness: parallel copies, offset across the ray
      const bool vertical = q[0] == q[2];
      const int16_t ox = vertical ? k : 0, oy = vertical ? 0 : k;
      c.drawLine(cx + q[0] + ox, cy + q[1] + oy, cx + q[2] + ox, cy + q[3] + oy, ink);
    }
}

static void cloudShape(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, int16_t inset, lb_color_t col) {
  const int16_t base = cy + s / 5;                // flat bottom
  const int16_t r1 = s / 5 - inset, r2 = s / 4 - inset, r3 = s / 6 - inset;
  c.fillCircle(cx - s / 4, base - s / 5, r1, col);
  c.fillCircle(cx + s / 20, base - s / 4, r2, col);
  c.fillCircle(cx + s / 3 - s / 12, base - s / 6, r3, col);
  c.fillRect(cx - s / 4, base - s / 5 + inset, s / 2 + s / 12, s / 5 - inset * 2 + 1, col);
  c.fillCircle(cx - s / 4, base - r1 - inset, r1, col);
}

static void cloud(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  cloudShape(c, cx, cy, s, 0, ink);
  cloudShape(c, cx, cy, s, stroke(s), paper);
}

static void drop(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s), r = s / 4, by = cy + s / 8;
  const int16_t top = cy - s / 2 + t;
  c.fillCircle(cx, by, r, ink);
  c.fillTriangle(cx, top, cx - r + 1, by - r / 3, cx + r - 1, by - r / 3, ink);
  c.fillCircle(cx, by, r - t, paper);
  c.fillTriangle(cx, top + t * 2 + 1, cx - r + t + 1, by - r / 3, cx + r - t - 1, by - r / 3, paper);
}

/* level 0..100 */
static void thermometer(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, int16_t level,
                        lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s), tw = s / 5, br = s / 6;
  const int16_t top = cy - s / 2 + 1, by = cy + s / 2 - br - 1;
  c.fillRoundRect(cx - tw / 2, top, tw, by - top, tw / 2, ink);
  c.fillCircle(cx, by, br, ink);
  c.fillRoundRect(cx - tw / 2 + t, top + t, tw - 2 * t, by - top, tw / 2, paper);
  c.fillCircle(cx, by, br - t, paper);
  // the mercury: bulb plus a column whose height is the level
  const int16_t inner = tw - 4 * t < 1 ? 1 : tw - 4 * t;
  const int16_t colTop = by - (int32_t)(by - top - 2 * t) * level / 100;
  c.fillRect(cx - inner / 2, colTop, inner, by - colTop, ink);
  c.fillCircle(cx, by, br - 2 * t, ink);
}

/* level 0..100, drawn as up to four bars */
static void battery(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, int16_t level,
                    lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s), w = s - s / 8, h = s / 2, x = cx - s / 2, y = cy - h / 2;
  c.fillRect(x, y, w, h, ink);
  c.fillRect(x + t, y + t, w - 2 * t, h - 2 * t, paper);
  c.fillRect(x + w, cy - h / 4, s / 8, h / 2, ink);  // the terminal nub
  const int16_t gap = t + 1, bars = (level + 24) / 25;
  const int16_t bw = (w - 2 * t - 5 * gap) / 4;
  for (int16_t i = 0; i < bars && i < 4; i++)
    c.fillRect(x + t + gap + i * (bw + gap), y + t + gap, bw, h - 2 * (t + gap), ink);
}

static void wifi(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s) + (s >= 24 ? 1 : 0), by = cy + s / 3;
  // Three rings round the same centre, then everything outside a 90-degree
  // wedge painted over. What is left are three arcs.
  for (int16_t i = 3; i >= 1; i--) ring(c, cx, by, s * i / 6 + t / 2, t, ink, paper);
  const int16_t R = s / 2 + t + 1;
  c.fillTriangle(cx, by, cx - R, by - R + 1, cx - R, by + R, paper);
  c.fillTriangle(cx, by, cx + R, by - R + 1, cx + R, by + R, paper);
  c.fillRect(cx - R, by + 1, 2 * R + 1, R, paper);
  c.fillCircle(cx, by - t / 2, t + 1, ink);          // the dot
}

static void house(LB_Canvas &c, int16_t cx, int16_t cy, int16_t s, lb_color_t ink = LB_BLACK, lb_color_t paper = LB_WHITE) {
  const int16_t t = stroke(s), half = s / 2, roofY = cy - s / 8;
  const int16_t top = cy - half + 1, left = cx - half + s / 8, right = cx + half - s / 8;
  c.fillTriangle(cx, top, cx - half + 1, roofY, cx + half - 1, roofY, ink);
  c.fillRect(left, roofY, right - left, cy + half - roofY - 1, ink);
  c.fillTriangle(cx, top + t * 2, cx - half + t * 2 + 1, roofY + t / 2, cx + half - t * 2 - 1, roofY + t / 2, paper);
  c.fillRect(left + t, roofY, right - left - 2 * t, cy + half - roofY - 1 - t, paper);
  c.fillRect(cx - s / 10, cy + half - s / 3, s / 5, s / 3 - 1, ink);   // the door
}

}  // namespace Icons
