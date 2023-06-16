#include "../include/list.h"
#include "../include/polygon.h"
#include "../include/sdl_wrapper.h"
#include "../include/state.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

const float GRAVITY_ACCEL = -9.8;
const float DAMPING_FACTOR =
    .91; // proportion of energy/speed conserved after every bounce
const float INNER_RADIUS =
    20; // distance from center of star to each interior polygon point.
const float HEIGHT_LEN = 37; // number of pixels from base of triangle
                             // representing a star corner to its tip.
const vector_t INITIAL_VEL = {10, 0};
const vector_t INITIAL_POS = {100, 450}; // top left corner w/ space for the
                                         // star
const double DUMMY_MASS = 1;
const float INITIAL_ANGLE = 0;
const float ANG_VEL = 3;
const int FPS = 5;
const float SPAWN_PERIOD = 3.5; // how often stars spawn in seconds

/**
 * A struct to represent the current state of the program.
 */
typedef struct state {
  scene_t *scene;
  size_t curr_sides;
  float time_since_last_star;
} state_t;

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
list_t *make_star_shape(size_t num_corners, double corner_base_len,
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

body_t *star_init(state_t *state) {
  body_t *star;

  rgb_color_t color = {(float)rand() / (double)RAND_MAX,
                       (float)rand() / (double)RAND_MAX,
                       (float)rand() / (double)RAND_MAX};

  // Edge case for curr_sides = 2: increase height_len to make
  // it proportional to the rest of the stars
  if (state->curr_sides == 2) {
    star = body_init(make_star_shape(state->curr_sides, INNER_RADIUS,
                                     HEIGHT_LEN + INNER_RADIUS / 2),
                     DUMMY_MASS, color);
  } else {
    star =
        body_init(make_star_shape(state->curr_sides, INNER_RADIUS, HEIGHT_LEN),
                  DUMMY_MASS, color);
  }

  body_set_velocity(star, INITIAL_VEL);

  // Translate and initial position and rotate to initial angle
  body_set_centroid(star, INITIAL_POS);
  body_set_rotation(star, INITIAL_ANGLE);

  return star;
}

/**
 * Initialize program instance.
 *
 * @return the initial state of the program
 */
vector_t calculate_translation(vector_t vel, double time) {
  // Using kinematic formulas (1/2at^2)
  double y_pos = vel.y * (time) + 0.5 * GRAVITY_ACCEL * (time) * (time);
  double x_pos = vel.x * time;
  vector_t translation = {x_pos, y_pos};
  return translation;
}

/**
 * Checks if a star is bouncing or is out of the screen.
 *
 * @param star a pointer to a a struct for a star
 * @param state a pointer to the current state of the program
 */
void check_out_of_bounds(body_t *star, state_t *state) {
  int dpixel = 5; // number of pixels to shift star into boundaries if collided
  bool is_disappeared = true;
  for (size_t i = 0; i < list_size(body_get_shape(star)); i++) {
    vector_t p = *(vector_t *)list_get(body_get_shape(star), i);

    // Check if star is too far down, if so shift polygon into
    // boundaries and signal to flip y velocity
    if (p.y <= 0) {
      vector_t new_centroid = {body_get_centroid(star).x,
                               body_get_centroid(star).y - p.y + dpixel};
      body_set_centroid(star, new_centroid);

      vector_t new_vel = {body_get_velocity(star).x,
                          -body_get_velocity(star).y * DAMPING_FACTOR};

      body_set_velocity(star, new_vel);
    }
    // Checks if any points in the star are still on the screen
    if (p.x <= WINDOW.x) {
      is_disappeared = false;
    }
  }

  // Remove the first star in the list
  if (is_disappeared) {
    scene_remove_body(state->scene, 0);
  }
}

/**
 * Initialize program instance.
 *
 * @return a pointer to the the initial state of the program
 */
state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  // Randomize the seed each time a star is initalized
  srand(time(NULL));

  state_t *state = calloc(1, sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;
  state->curr_sides = 2;
  state->time_since_last_star = 0.0;

  // Initialize the first star
  scene_add_body(state->scene, star_init(state));

  return state;
}

/**
 * Update screen at each frame.
 *
 * @param state a pointer to the current state of the program
 * @return a pointer to the current state of the program
 */
void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();

  // Track how long until next star is generated
  state->time_since_last_star += dt;

  double time_factor = FPS * dt; // multiplied to translation and rotation to
                                 // ensure program behaves same in all frames

  // Spawns a new star after a constant time period
  if (state->time_since_last_star >= SPAWN_PERIOD) {
    state->curr_sides++;
    scene_add_body(state->scene, star_init(state));
    state->time_since_last_star = 0;
  }

  // Calculate movement of each star
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *curr_star = scene_get_body(state->scene, i);

    // vector_t translation =
    // calculate_translation(body_get_velocity(curr_star), time_factor);
    double rotation = deg_to_rad(ANG_VEL) * time_factor;

    vector_t new_vel = {body_get_velocity(curr_star).x,
                        body_get_velocity(curr_star).y +
                            GRAVITY_ACCEL * time_factor};

    body_set_velocity(curr_star, new_vel);

    body_set_rotation(curr_star, rotation);

    check_out_of_bounds(curr_star, state);
  }

  scene_tick(state->scene, time_factor);

  sdl_render_scene(state->scene);
}

/**
 * Free memory allocated to the state and contents within.
 */
void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}
