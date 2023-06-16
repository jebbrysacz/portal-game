#ifndef __COLOR_H__
#define __COLOR_H__

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components.
 * Each component must be between 0 (black) and 1 (white).
 */
typedef struct {
  float r;
  float g;
  float b;
} rgb_color_t;

/**
 * A function to convert HSV color values to an RBG color.
 *
 * @param h hue
 * @param s saturation
 * @param v value
 */
rgb_color_t hsv_to_rgb(float h, float s, float v);

#endif // #ifndef __COLOR_H__
