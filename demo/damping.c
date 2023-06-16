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

const double CIRCLE_RADIUS = 10;
const double CIRCLE_MASS = 10;
const size_t NUM_CIRCLES = 50;
const double TOTAL_CIRCLE_ANGLE = 360;
const double K = 10;
const double GAMMA_FACTOR = .1;
const double BASE_GAMMA = .01;

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
 * Generates a circle shape with an inputted radius at the given position.
 *
 * @param radius radius of circle
 * @return pointer that points to the pellet
 */
list_t *make_circle_shape(double radius) {
  list_t *circle_shape = list_init(TOTAL_CIRCLE_ANGLE, free);

  double curr_angle = 0.0;
  double dt = 1; // change in angle in each point

  for (size_t i = 0; i < TOTAL_CIRCLE_ANGLE; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = radius * cos(deg_to_rad(curr_angle));
    new_point->y = radius * sin(deg_to_rad(curr_angle));
    list_add(circle_shape, new_point);

    curr_angle += dt;
  }

  return circle_shape;
}

/**
 * Initializes a circle body with color and mass.
 *
 * @param initial_pos the initial position of the centroid of the circle
 * @param color the color RGB of the circle
 * @param mass the mass of the circle
 * @return a pointer to the initialized circle body
 */
body_t *make_circle_body(vector_t initial_pos, rgb_color_t color, double mass) {
  body_t *circle = body_init(make_circle_shape(CIRCLE_RADIUS), mass, color);

  body_set_centroid(circle, initial_pos);

  return circle;
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

  // Add anchors, circles, and forces
  double dh = 360.0 / NUM_CIRCLES; // change in hue
  for (size_t i = 0; i < NUM_CIRCLES; i++) {
    // Add anchors
    vector_t anchor_pos = {CIRCLE_RADIUS * (2 * i + 1), WINDOW.y / 2};
    rgb_color_t anchor_color = {0, 0, 0};
    body_t *anchor_body = make_circle_body(anchor_pos, anchor_color, INFINITY);
    scene_add_body(scene, anchor_body);

    // Add circles
    double x = CIRCLE_RADIUS * (2 * i + 1);
    double y = WINDOW.y / 2 * cos((2 * M_PI / WINDOW.x) * x) + WINDOW.y / 2;
    double h = i * dh;
    vector_t circle_initial_pos = {x, y};
    rgb_color_t circle_color = hsv_to_rgb(h, 1, 1);
    body_t *circle_body =
        make_circle_body(circle_initial_pos, circle_color, CIRCLE_MASS);
    scene_add_body(scene, circle_body);

    // Create forces between anchors, drag on bodies
    create_spring(scene, K, circle_body, anchor_body);
    create_drag(scene, BASE_GAMMA + GAMMA_FACTOR * i, circle_body);
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