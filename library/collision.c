#include "../include/collision.h"
#include "../include/list.h"
#include "../include/vector.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

vector_t compute_normal(vector_t v1, vector_t v2) {
  vector_t edge = vec_subtract(v1, v2);
  double dist = sqrt(vec_dot(edge, edge));
  vector_t unit = vec_multiply(1.0 / dist, edge);
  vector_t normal = {-unit.y, unit.x};
  return normal;
}

double *find_min_max_interval(list_t *shape, vector_t normal) {
  double *interval = calloc(2, sizeof(double));
  // Min: "left-most" projection on normal line
  // Max: "right-most" projection on normal line

  double min = 1.0 / 0.0;  //  inf
  double max = -1.0 / 0.0; // -inf
  for (size_t i = 0; i < list_size(shape); i++) {
    vector_t v = *(vector_t *)list_get(shape, i);
    double proj = vec_dot(v, normal);
    if (proj < min) {
      min = proj;
    }
    if (proj > max) {
      max = proj;
    }
  }
  interval[0] = min;
  interval[1] = max;
  return interval;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  collision_info_t collision_info;

  double shortest_overlap = 1.0 / 0.0;

  list_t *shape;
  list_t *other_shape;
  for (size_t i = 0; i < list_size(shape1) + list_size(shape2); i++) {
    size_t j;
    if (i < list_size(shape1)) {
      j = i;
      shape = shape1;
      other_shape = shape2;
    } else {
      j = i - list_size(shape1);
      shape = shape2;
      other_shape = shape1;
    }
    vector_t v1 = *(vector_t *)list_get(shape, j);
    vector_t v2 = *(vector_t *)list_get(shape, (j + 1) % list_size(shape));
    vector_t normal = compute_normal(v1, v2);

    double *interval1 = find_min_max_interval(shape, normal);
    double *interval2 = find_min_max_interval(other_shape, normal);
    double min1 = interval1[0];
    double min2 = interval2[0];
    double max1 = interval1[1];
    double max2 = interval2[1];
    free(interval1);
    free(interval2);

    if (max1 < min2 || max2 < min1) {
      collision_info.collided = false;
      return collision_info;
    }

    double overlap = fabs(fmax(min1, min2) - fmin(max1, max2));
    if (overlap < shortest_overlap) {
      shortest_overlap = overlap;
      collision_info.axis = normal;
    }
  }
  collision_info.collided = true;
  return collision_info;
}
