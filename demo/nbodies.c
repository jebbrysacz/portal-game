#include "../include/body.h"
#include "../include/forces.h"
#include "../include/list.h"
#include "../include/polygon.h"
#include "../include/scene.h"
#include "../include/sdl_wrapper.h"
#include "../include/vector.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

const size_t NUM_BODIES = 100;
const size_t NUM_SIDES = 4;
const double BASE_MASS = 10;
const double BASE_INNER_RADIUS = 5;
const double BASE_HEIGHT_LEN = 6;
const vector_t INITIAL_VEL = {0, 0};
const double MAX_MASS = 20;
const double G = 1e3;
const vector_t SPAWNING_WINDOW = {600, 300};
const double TOTAL_CIRCLE_ANGLE = 360;

/**
 * A struct to represent the current state of the program.
 */
typedef struct state {
  scene_t *scene;
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
  double t1 = (num_corners - 2) * TOTAL_CIRCLE_ANGLE /
              2;                      // total inner angle of interior polygon
                                      // (i.e. pentagon for 5 corner star
                                      // with t1 = 540)
  double t2 = t1 / (2 * num_corners); // half of angle of one corner of interior
                                      // polygon (i.e. 54 for 5 corner star)
  double t3 =
      TOTAL_CIRCLE_ANGLE / num_corners; // arc angle for each edge of interior
                                        // polygon (i.e. 72 for 5 corner star)
  double t4 =
      TOTAL_CIRCLE_ANGLE / 4; // initial angle of star corner point north of
                              // x-axis (i.e. first star point points up)

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

/**
 * Initialize a star body.
 *
 * @param mass the mass of the star.
 */
body_t *make_star_body(double mass) {
  body_t *star;

  double ratio = mass / BASE_MASS; // ratio between inputted mass and base_mass

  // Calculate inner_radius and height_len proportionally to base values
  double inner_radius = ratio * BASE_INNER_RADIUS;
  double height_len = ratio * BASE_HEIGHT_LEN;

  // Generate random color and random initial position
  rgb_color_t color = {(float)rand() / (double)RAND_MAX,
                       (float)rand() / (double)RAND_MAX,
                       (float)rand() / (double)RAND_MAX};
  double initial_x =
      rand() % (int)SPAWNING_WINDOW.x + (WINDOW.x - SPAWNING_WINDOW.x) / 2;
  double initial_y =
      rand() % (int)SPAWNING_WINDOW.y + (WINDOW.y - SPAWNING_WINDOW.y) / 2;
  vector_t initial_pos = {initial_x, initial_y};

  star = body_init(make_star_shape(NUM_SIDES, inner_radius, height_len), mass,
                   color);

  body_set_velocity(star, INITIAL_VEL);
  body_set_centroid(star, initial_pos);

  return star;
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

  // scene_add_body(scene, make_star_body(100));

  // Add bodies into scene
  for (size_t i = 0; i < NUM_BODIES; i++) {
    double random_mass = rand() % (int)MAX_MASS;
    scene_add_body(scene, make_star_body(random_mass));
  }

  // Add gravitational forces between every body
  for (size_t i = 0; i < scene_bodies(scene) - 1; i++) {
    for (size_t j = i + 1; j < scene_bodies(scene); j++) {
      create_newtonian_gravity(scene, G, scene_get_body(scene, i),
                               scene_get_body(scene, j));
    }
  }

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

  scene_tick(state->scene, dt);

  sdl_render_scene(state->scene);
}

/**
 * Free memory allocated to the state and contents within.
 */
void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}