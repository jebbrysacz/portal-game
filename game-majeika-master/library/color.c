#include "../include/color.h"
#include <math.h>

rgb_color_t hsv_to_rgb(float h, float s, float v) {
  double c = v * s;
  double x = c * (1 - fabs(remainder(h / 60, 2) - 1));
  double m = v - c;

  float r = 0;
  float g = 0;
  float b = 0;
  if (0 <= h && h < 60) {
    r = c;
    g = x;
    b = 0;
  } else if (60 <= h && h < 120) {
    r = x;
    g = c;
    b = 0;
  } else if (120 <= h && h < 180) {
    r = 0;
    g = c;
    b = x;
  } else if (180 <= h && h < 240) {
    r = 0;
    g = x;
    b = c;
  } else if (240 <= h && h < 300) {
    r = x;
    g = 0;
    b = c;
  } else if (300 <= h && h < 360) {
    r = c;
    g = 0;
    b = x;
  }

  r += m;
  g += m;
  b += m;

  return (rgb_color_t){fabs(r), fabs(g), fabs(b)};
}