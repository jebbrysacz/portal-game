#include "../include/shapes.h"
#include "../include/vector.h"
#include <stdlib.h>

const double TOTAL_CIRCLE_ANGLE = 360;

double deg_to_rad(double deg) { return deg * M_PI / 180; }

list_t *make_rect_shape(double width, double height) {
  // Initialize the brick shape
  list_t *shape = list_init(4, free);

  vector_t *v = malloc(sizeof(vector_t));
  *v = (vector_t){0, 0};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){width, 0};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){width, height};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){0, height};
  list_add(shape, v);

  return shape;
}

list_t *make_circ_shape(double radius, size_t num_points) {
  list_t *shape = list_init(TOTAL_CIRCLE_ANGLE, free);

  double curr_angle = 0;
  double dt = TOTAL_CIRCLE_ANGLE/num_points; // change in angle in each point

  for (size_t i = 0; i < num_points; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = radius * cos(deg_to_rad(curr_angle));
    new_point->y = radius * sin(deg_to_rad(curr_angle));
    list_add(shape, new_point);

    curr_angle += dt;
  }

  return shape;
}