#include "../include/body.h"
#include "../include/color.h"
#include "../include/list.h"
#include "../include/polygon.h"
#include "../include/sdl_wrapper.h"
#include "../include/shapes.h"
#include "../include/vector.h"
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct body {
  list_t *shape;
  rgb_color_t color;
  double mass;
  vector_t vel;
  vector_t centroid;
  vector_t force;
  vector_t impulse;
  void *info;
  free_func_t info_freer;
  bool is_removed;
  double rotation;
  SDL_Texture *text;
  SDL_Texture *image;
  const char *image_path;
  bool is_visible;
} body_t;

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  return body_init_with_info(shape, mass, color, NULL, NULL);
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
  return body_init_with_image(shape, mass, color, info, info_freer, NULL);
}

body_t *body_init_with_image(list_t *shape, double mass, rgb_color_t color,
                             void *info, free_func_t info_freer,
                             const char *image_path) {
  body_t *new_body = calloc(1, sizeof(body_t));
  assert(new_body);
  new_body->shape = shape;
  new_body->color = color;
  new_body->mass = mass;
  new_body->vel = (vector_t){0.0, 0.0};
  new_body->centroid = polygon_centroid(shape);
  new_body->force = (vector_t){0.0, 0.0};
  new_body->impulse = (vector_t){0.0, 0.0};
  new_body->info = info;
  new_body->info_freer = info_freer;
  new_body->is_removed = false;
  new_body->rotation = 0;
  new_body->text = NULL;
  if (image_path) {
    new_body->image = sdl_load_image(image_path);
  } else {
    new_body->image = NULL;
  }
  new_body->image_path = image_path;
  new_body->is_visible = true;

  return new_body;
}

void body_free(body_t *body) {
  list_free(body->shape);
  if (body->info_freer && body->info) {
    body->info_freer(body->info);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) {
  list_t *shape_copy = list_init(list_size(body->shape), free);
  for (size_t i = 0; i < list_size(body->shape); i++) {
    vector_t *new_p = malloc(sizeof(vector_t));
    *new_p = *(vector_t *)list_get(body->shape, i);
    list_add(shape_copy, new_p);
  }
  return shape_copy;
}

vector_t body_get_centroid(body_t *body) { return body->centroid; }

vector_t body_get_velocity(body_t *body) { return body->vel; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void *body_get_info(body_t *body) { return body->info; }

double body_get_mass(body_t *body) { return body->mass; }

vector_t body_get_force(body_t *body) { return body->force; }

vector_t body_get_impulse(body_t *body) { return body->impulse; }

double body_get_rotation(body_t *body) { return body->rotation; }

SDL_Texture *body_get_text(body_t *body) { return body->text; }

SDL_Texture *body_get_image(body_t *body) { return body->image; }

bool body_get_is_visible(body_t *body) { return body->is_visible; }

void body_set_visibility(body_t *body, bool is_visible) {
  body->is_visible = is_visible;
}

void body_set_text(body_t *body, SDL_Texture *text) { body->text = text; }

void body_set_image(body_t *body, SDL_Texture *image) { body->image = image; }

void body_set_centroid(body_t *body, vector_t x) {
  vector_t translation = vec_subtract(x, body->centroid);
  polygon_translate(body->shape, translation);
  body->centroid = x;
}

void body_set_velocity(body_t *body, vector_t v) { body->vel = v; }

void body_set_rotation_around_point(body_t *body, double angle,
                                    vector_t point) {
  body->rotation += angle;
  polygon_rotate(body->shape, angle, point);
}

void body_set_rotation(body_t *body, double angle) {
  body_set_rotation_around_point(body, angle, body_get_centroid(body));
}

void body_add_force(body_t *body, vector_t force) {
  body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->impulse = vec_add(body->impulse, impulse);
}

void body_tick(body_t *body, double dt) {
  vector_t acceleration = vec_multiply(1 / body->mass, body->force);
  vector_t new_vel = vec_add(body->vel, vec_multiply(dt, acceleration));
  new_vel = vec_add(new_vel, vec_multiply(1 / body->mass, body->impulse));
  vector_t avg_vel = vec_multiply(0.5, vec_add(new_vel, body->vel));

  vector_t new_centroid = vec_add(body->centroid, vec_multiply(dt, avg_vel));

  body_set_centroid(body, new_centroid);
  body_set_velocity(body, new_vel);

  body->force = (vector_t){0, 0};
  body->impulse = (vector_t){0, 0};
}

void body_remove(body_t *body) { body->is_removed = true; }

bool body_is_removed(body_t *body) { return body->is_removed; }
