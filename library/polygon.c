#include "../include/polygon.h"
#include <math.h>
#include <stdlib.h>

double polygon_area(list_t *polygon) {
  /*
    The function computes the area of the polygon using this formula:

    A = (1/2) * \sum_{i=0}^{n-1} (y_i + y_{i+1}) * (x_i - x_{i+1})
  */

  double sum_area = 0;
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t curr = *(vector_t *)list_get(polygon, i);
    vector_t next =
        *(vector_t *)list_get(polygon, (i + 1) % list_size(polygon));
    sum_area += (curr.y + next.y) * (curr.x - next.x);
  }

  return sum_area / 2;
}

vector_t polygon_centroid(list_t *polygon) {
  /*
    The function computes the centroid of the polygon using this formula:

    c_x = (1/6A) * \sum_{i=0}^{n-1} (x_i + x_{i+1})*(x_i * y_{i+1} - x_{i+1} *
    y_i) c_y = (1/6A) * \sum_{i=0}^{n-1} (y_i + y_{i+1})*(x_i * y_{i+1} -
    x_{i+1} * y_i)

    where the A is the area of the polygon and the centroid is c = (c_x, c_y)
  */

  double area = polygon_area(polygon);
  double c_x = 0;
  double c_y = 0;
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t curr = *(vector_t *)list_get(polygon, i);
    vector_t next =
        *(vector_t *)list_get(polygon, (i + 1) % list_size(polygon));
    c_x += (curr.x + next.x) * ((curr.x) * (next.y) - (next.x) * (curr.y));
    c_y += (curr.y + next.y) * ((curr.x) * (next.y) - (next.x) * (curr.y));
  }

  vector_t c = {c_x / (6 * area), c_y / (6 * area)};
  return c;
}

void polygon_translate(list_t *polygon, vector_t translation) {
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t *v = list_get(polygon, i);
    *v = vec_add(*v, translation);
  }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
  // Translate polygon to origin
  polygon_translate(polygon, vec_negate(point));

  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t *v = list_get(polygon, i);
    *v = vec_rotate(*v, angle);
  }

  // Translate polygon back to original point
  polygon_translate(polygon, point);
}