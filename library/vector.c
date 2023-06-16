#include "../include/vector.h"
#include <math.h>
#include <stdio.h>

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t output_v = {v1.x + v2.x, v1.y + v2.y};
  return output_v;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  vector_t output_v = {v1.x - v2.x, v1.y - v2.y};
  return output_v;
}

vector_t vec_negate(vector_t v) { return vec_multiply(-1, v); }

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t output_v = {v.x * scalar, v.y * scalar};
  return output_v;
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
  double new_x = v.x * cos(angle) - v.y * sin(angle);
  double new_y = v.x * sin(angle) + v.y * cos(angle);
  v.x = new_x;
  v.y = new_y;
  return v;
}

double vec_direction_angle(vector_t v) {
  // Get rid of -0 values
  if (fabs(v.x) <= 1e-3) {
    v.x = 0;
  }
  if (fabs(v.y) <= 1e-3) {
    v.y = 0;
  }

  if (v.x >= 0) {
    if (v.y > 0) {
      return atan(v.y / v.x);
    } else if (v.y < 0) {
      return 2 * M_PI + atan(v.y / v.x);
    } else {
      return 0;
    }
  } else {
    if (v.y > 0) {
      return M_PI + atan(v.y / v.x);
    } else if (v.y < 0) {
      return M_PI + atan(v.y / v.x);
    } else {
      return M_PI;
    }
  }
}