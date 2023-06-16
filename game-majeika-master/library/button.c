#include "../include/button.h"
#include "../include/body_type.h"
#include "../include/collision.h"
#include "../include/platform.h"
#include "../include/shapes.h"
#include <stdio.h>

typedef struct button {
  body_t *button_body;
  body_t *base_body;
  list_t *platforms;
  bool is_pressed;
  double sum_motion_time;
  double total_motion_time;
  vector_t total_press_translation;
} button_t;

/**
 * Allocate memory for a pointer to a body representing the part of 
 * the button that will get pressed.
 * 
 * @param pos a vector describing the button's position. pos.x is the center and
 * pos.y is the bottom-most point of the button.
 * @param button_dims a vector describing the button's x and y dimensions
 * @param base_dims a vector describing the button base's x and y dimensions
 * @param button_color the button's color
 * @return a pointer to the newly allocated button body
 */
body_t *create_button_body(vector_t pos, vector_t button_dims,
                           vector_t base_dims, rgb_color_t button_color) {
  list_t *button_shape = make_rect_shape(button_dims.x, button_dims.y);
  body_t *button_body = body_init_with_info(
      button_shape, INFINITY, button_color, make_type_info(BUTTON), free);

  vector_t centroid = {pos.x, pos.y + button_dims.y / 2 + base_dims.y};
  body_set_centroid(button_body, centroid);

  return button_body;
}

/**
 * Allocate memory for a pointer to a body representing the base of 
 * the button.
 * 
 * @param pos a vector describing the button's position. pos.x is the center and
 * pos.y is the bottom-most point of the button.
 * @param base_dims a vector describing the button base's x and y dimensions
 * @param base_color the base's color
 * @return a pointer to the newly allocated button body
 */
body_t *create_base_body(vector_t pos, vector_t base_dims,
                         rgb_color_t base_color) {
  list_t *base_shape = make_rect_shape(base_dims.x, base_dims.y);
  body_t *base_body = body_init(base_shape, INFINITY, base_color);

  vector_t centroid = {pos.x, pos.y + base_dims.y / 2};
  body_set_centroid(base_body, centroid);

  return base_body;
}

button_t *button_init(vector_t pos, vector_t button_dims,
                      rgb_color_t button_color, vector_t base_dims,
                      rgb_color_t base_color, list_t *platforms) {
  button_t *button = calloc(1, sizeof(button_t));
  button->button_body =
      create_button_body(pos, button_dims, base_dims, button_color);
  button->base_body = create_base_body(pos, base_dims, base_color);
  button->platforms = platforms;
  button->is_pressed = false;
  button->sum_motion_time = 0;
  button->total_motion_time = .25;
  button->total_press_translation = (vector_t){0, -0.8 * button_dims.y};

  return button;
}

void button_free(button_t *button) {
  list_free(button->platforms);
  free(button);
}

body_t *button_get_button_body(button_t *button) { return button->button_body; }

body_t *button_get_base_body(button_t *button) { return button->base_body; }

/**
 * Press the button to its specified final state.
 * 
 * @param button a pointer to the button
 * @param dt the number of seconds elapsed since the last tick
 */
void button_press(button_t *button, double dt) {
  button->sum_motion_time += dt;
  if (button->sum_motion_time < 0) {
    button->sum_motion_time = -1e-5;
  } else if (button->sum_motion_time > button->total_motion_time) {
    button->sum_motion_time = button->total_motion_time + 1e-5;
  }

  if (0 <= button->sum_motion_time &&
      button->sum_motion_time <= button->total_motion_time) {

    body_t *button_body = button->button_body;
    double ratio = dt / button->total_motion_time;

    vector_t translation = vec_multiply(ratio, button->total_press_translation);
    vector_t new_centroid =
        vec_add(body_get_centroid(button_body), translation);
    body_set_centroid(button_body, new_centroid);
  }
}

/**
 * Reset the button to its specified final state.
 * 
 * @param button a pointer to the button
 * @param dt the number of seconds elapsed since the last tick
 */
void button_unpress(button_t *button, double dt) { button_press(button, -dt); }

void button_tick(button_t *button, list_t *pressing_bodies, double dt) {
  body_t *button_body = button->button_body;
  list_t *button_shape = body_get_shape(button_body);
  button->is_pressed = false;

  // Check whether or not button should be pressed
  for (size_t i = 0; i < list_size(pressing_bodies); i++) {
    list_t *pressing_shape = body_get_shape(list_get(pressing_bodies, i));
    collision_info_t collision_info =
        find_collision(button_shape, pressing_shape);

    if (collision_info.collided) {
      button->is_pressed = true;
      break;
    }

    list_free(pressing_shape);
  }
  list_free(button_shape);

  if (button->is_pressed) {
    button_press(button, dt);
  } else {
    button_unpress(button, dt);
  }

  // Move platforms
  for (size_t i = 0; i < list_size(button->platforms); i++) {
    platform_t *platform = list_get(button->platforms, i);
    if (button->is_pressed) {
      platform_change_is_moving(platform, true);
    } else {
      platform_change_is_moving(platform, false);
    }
    platform_tick(platform, dt);
  }
}