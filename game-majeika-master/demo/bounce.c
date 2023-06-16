#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../include/color.h"
#include "../include/polygon.h"
#include "../include/sdl_wrapper.h"
#include "../include/state.h"
#include <math.h>
#include <stdlib.h>

// window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

// Star color (rgb values normalized to 0-1)
const rgb_color_t COLOR = {1, 1, 0};

const size_t NUM_CORNERS = 5; // # of corners/sides the star has
// # of pixels the base of the triangle representing a corner has
const double BASE_LEN = 50;
// # of pixels the height of the triangle representing a corner has
const double HEIGHT_LEN = 100;
// Allows a user to set their own HEIGHT_LEN or make the star proportional
// If true, HEIGHT_LEN gets recalculated to make a proportional star
// If false, HEIGHT_LEN is that set on line 31
const bool PROPORTIONAL_STAR = true;
const vector_t INITIAL_POS = {500, 250};
const double INITIAL_ANGLE = 60;
const double ANG_VEL = 1;

/**
 * Convert an angle from degrees to radians.
 *
 * @param deg angle in degrees
 * @return angle in radians
 */
double deg_to_rad(double deg) { return deg * M_PI / 180; }

/**
 * Get the star corner point given the two interior polygon points before and
 * after the corner point.
 *
 * @param curr interior polygon point before the star corner point
 * @param next interior polygon point after the star corner point
 * @param corner_height_len number of pixels from base of triangle
 * representing a star corner to its tip.
 * @param t angle between positive x-axis and interior polygon edge
 * @return vector_t representing the star corner point
 */
vector_t get_corner_point(vector_t curr, vector_t next,
                          double corner_height_len, double t) {
  // Find the midpoint of the two interior polygon points
  vector_t corner_point = {(curr.x + next.x) / 2, (curr.y + next.y) / 2};

  // Add corner_height_len to corner_point in correct direction
  corner_point.x = corner_point.x + corner_height_len * cos(deg_to_rad(t));
  corner_point.y = corner_point.y + corner_height_len * sin(deg_to_rad(t));

  return corner_point;
}

/**
 * Generate the points that represent a specified star.
 *
 * @param num_corners the number of star corners or tips
 * (5 in generic star).
 * @param corner_base_len number of pixels for the base of triangle
 * representing a star corner.
 * @param corner_height_len number of pixels from base of triangle
 * representing a star corner to its tip.
 * @return list of vertex points representing the star polygon.
 */
list_t *make_star(size_t num_corners, double corner_base_len,
                  double corner_height_len) {
  // List of vertex points representing the star polygon
  // There are a total of 2*num_corners points because for each star corner,
  // there is an interior polygon point
  list_t *points = list_init(2 * num_corners, free);

  // Initialize each point as a zero vector_t
  for (size_t i = 0; i < 2 * num_corners; i++) {
    vector_t *v = malloc(sizeof(vector_t));
    v->x = 0;
    v->y = 0;
    list_add(points, v);
  }

  // Angles (in degrees)
  double t1 = (num_corners - 2) * 180; // total inner angle of interior polygon
                                       // (i.e. pentagon for 5 corner star
                                       // with t1 = 540)
  double t2 = t1 / (2 * num_corners); // half of angle of one corner of interior
                                      // polygon (i.e. 54 for 5 corner star)
  double t3 = 360 / num_corners; // arc angle for each edge of interior polygon
                                 // (i.e. 72 for 5 corner star)
  double t4 = 90; // initial angle of star corner point north of x-axis
                  // (i.e. first star point points up)

  // Find distance from center of star to each interior polygon point
  double d = corner_base_len / (2 * cos(deg_to_rad(t2)));

  // Add interior polygon points
  for (size_t i = 0; i < num_corners; i++) {
    vector_t v = {d * cos(deg_to_rad(t2)), d * sin(deg_to_rad(t2))};
    *(vector_t *)list_get(points, 2 * i) = v;
    t2 += t3;
  }

  // Add star corner points between each interior angle point
  for (size_t i = 0; i < num_corners; i++) {
    vector_t corner_point = get_corner_point(
        *(vector_t *)list_get(points, 2 * i),
        *(vector_t *)list_get(points, 2 * (i + 1) % list_size(points)),
        corner_height_len, t4);
    *(vector_t *)list_get(points, 2 * i + 1) = corner_point;
    t4 += t3;
  }

  return points;
}

typedef struct state {
  list_t *star;
  vector_t vel;
} state_t;

/**
 * Check if the star collided with the window boundaries.
 *
 * @param state the state of the program
 * @return a vector representing whether to flip/negate x and/or y velocity
 * component(s)
 */
vector_t check_collision(state_t *state) {
  vector_t flip = {0, 0}; // which velocity components to flip/negate
  int dpixel = 5; // number of pixels to shift star into boundaries if collided

  for (size_t i = 0; i < NUM_CORNERS; i++) {
    vector_t *p = list_get(state->star, 2 * i + 1); // star corner point

    // Check if star is too far left or too far right, if so shift polygon into
    // boundaries and signal to flip x velocity
    if (p->x <= 0) {
      vector_t translation = {-p->x + dpixel, 0};
      polygon_translate(state->star, translation);

      flip.x = 1;
    } else if (p->x >= WINDOW.x) {
      vector_t translation = {WINDOW.x - p->x - dpixel, 0};
      polygon_translate(state->star, translation);

      flip.x = 1;
    }

    // Do the same to the y-axis
    if (p->y <= 0) {
      vector_t translation = {0, -p->y + dpixel};
      polygon_translate(state->star, translation);

      flip.y = 1;
    } else if (p->y >= WINDOW.y) {
      vector_t translation = {0, WINDOW.y - p->y - dpixel};
      polygon_translate(state->star, translation);

      flip.y = 1;
    }
  }

  return flip;
}

/**
 * Initialize program instance.
 *
 * @return the initial state of the program
 */
state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));

  if (PROPORTIONAL_STAR) {
    // Calculate what HEIGHT_LEN to input to get a star with the right
    // proportions Only works with num_corners >= 5, if lower, input HEIGHT_LEN
    // manually
    double proportional_height_len =
        BASE_LEN * tan(deg_to_rad(360 / NUM_CORNERS)) / 2;
    state->star = make_star(NUM_CORNERS, BASE_LEN, proportional_height_len);
  } else {
    state->star = make_star(NUM_CORNERS, BASE_LEN, HEIGHT_LEN);
  }

  // Set the state
  state->vel = (vector_t){-1, 2};

  // Translate and initial position and rotate to initial angle
  polygon_translate(state->star, INITIAL_POS);
  polygon_rotate(state->star, INITIAL_ANGLE, polygon_centroid(state->star));

  return state;
}

/**
 * Update screen at each frame.
 *
 * @param state the state of the program
 * @return the current state of the program
 */
void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  double time_factor = 50 * dt; // multiplied to translation and rotation to
                                // ensure program behaves same in all frames

  vector_t translation = {state->vel.x * time_factor,
                          state->vel.y * time_factor};
  double rotation = deg_to_rad(ANG_VEL) * time_factor;

  // translate_star(state, translation);
  polygon_translate(state->star, translation);
  polygon_rotate(state->star, rotation, polygon_centroid(state->star));

  vector_t flip = check_collision(state);

  if (flip.x == 1) {
    state->vel.x *= -1;
  }
  if (flip.y == 1) {
    state->vel.y *= -1;
  }

  sdl_draw_polygon(state->star, COLOR);
  sdl_show();
}

/**
 * Free memory allocated to the state and contents within.
 */
void emscripten_free(state_t *state) {
  list_free(state->star);
  free(state->star);
}