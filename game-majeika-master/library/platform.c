#include "../include/platform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct platform {
  body_t *body;
  double total_motion_time;
  double total_motion_angle;
  vector_t total_motion_translation;
  vector_t point_of_rotation;
  double sum_motion_time;
  bool is_moving;
} platform_t;

platform_t *platform_init(body_t *body, double total_motion_time,
                          double total_motion_angle,
                          vector_t total_motion_translation,
                          vector_t point_of_rotation) {
  platform_t *platform = calloc(1, sizeof(platform_t));
  platform->body = body;
  platform->total_motion_time = total_motion_time;
  platform->total_motion_angle = total_motion_angle;
  platform->total_motion_translation = total_motion_translation;
  platform->point_of_rotation = point_of_rotation;
  platform->sum_motion_time = 0;
  platform->is_moving = false;

  return platform;
}

bool platform_get_is_moving(platform_t *platform) {
  return platform->is_moving;
}

void platform_change_is_moving(platform_t *platform, bool is_moving) {
  platform->is_moving = is_moving;
}

/**
 * Move the platform to its specified final state.
 * 
 * @param platform a pointer to the platform
 * @param dt the number of seconds elapsed since the last tick
 */
void platform_move(platform_t *platform, double dt) {
  platform->sum_motion_time += dt;
  // Restrict sum_motion_time to near out of bounds
  if (platform->sum_motion_time < 0) {
    platform->sum_motion_time = -1e-5;
  } else if (platform->sum_motion_time > platform->total_motion_time) {
    platform->sum_motion_time = platform->total_motion_time + 1e-5;
  }

  if (0 <= platform->sum_motion_time &&
      platform->sum_motion_time <= platform->total_motion_time) {
    body_t *platform_body = platform->body;
    double ratio = dt / platform->total_motion_time;

    vector_t translation =
        vec_multiply(ratio, platform->total_motion_translation);
    vector_t new_centroid =
        vec_add(body_get_centroid(platform_body), translation);
    body_set_centroid(platform_body, new_centroid);

    double rotation_angle = platform->total_motion_angle * ratio;
    body_set_rotation_around_point(platform_body, rotation_angle,
                                   platform->point_of_rotation);
  }
}

/**
 * Reset the platform to its specified initial state.
 * 
 * @param platform a pointer to the platform
 * @param dt the number of seconds elapsed since the last tick
 */
void platform_reset(platform_t *platform, double dt) {
  platform_move(platform, -dt);
}

void platform_tick(platform_t *platform, double dt) {
  if (platform->is_moving) {
    platform_move(platform, dt);
  } else {
    platform_reset(platform, dt);
  }
}