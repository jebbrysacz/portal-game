#include "../include/body.h"
#include "../include/scene.h"
#include "../include/sdl_wrapper.h"
#include "../include/state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct state {
  scene_t *scene;
  double time_since_last_pellet;
  double pacman_direction;
} state_t;

// Window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

const double DUMMY_MASS = 1;

const double PACMAN_RADIUS = 50;
const size_t PACMAN_NUM_POINTS = 300;
const double PACMAN_INITIAL_MOUTH_ANGLE = 60; // (degrees)
const rgb_color_t PACMAN_COLOR = {1, 1, 0};
const vector_t PACMAN_INITIAL_POS = {500, 250};
const double PACMAN_INITIAL_DIRECTION = 0; // angle (degrees)

const double PELLET_RADIUS = 5;
const size_t PELLET_NUM_POINTS = 50;
const double PELLET_SPAWN_INTERVAL = 3; // seconds
const rgb_color_t PELLET_COLOR = {0, 1, 0};
const size_t PELLET_INITIAL_COUNT = 20;

const double ZERO_ACCEL_SPEED = 50; // speed if no key is being pressed
const double ACCELERATION = 100;    // pixels/s^2
const double TERMINAL_SPEED = 2000; // maximum speed of pacman

/**
 * Convert an angle from degrees to radians.
 *
 * @param deg angle in degrees
 * @return angle in radians
 */
double deg_to_rad(double deg) { return deg * M_PI / 180; }

/**
 * Make a new pacman shape centered at the given position with its mouth open
 * to the given angle.
 *
 * @param initial_pos starting position in the window
 * @param mouth_angle the angle at which to open the mouth (degrees)
 * @returns pointer that point to the pacman body
 */
body_t *make_pacman(vector_t initial_pos, double mouth_angle) {
  list_t *pacman_shape = list_init(PACMAN_NUM_POINTS + 1, free);

  double curr_angle = mouth_angle / 2;
  double dt =
      (360 - mouth_angle) / PACMAN_NUM_POINTS; // change in angle in each point

  for (size_t i = 0; i < PACMAN_NUM_POINTS; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = PACMAN_RADIUS * cos(deg_to_rad(curr_angle));
    new_point->y = PACMAN_RADIUS * sin(deg_to_rad(curr_angle));
    list_add(pacman_shape, new_point);

    curr_angle += dt;
  }

  // Add center point to body
  vector_t *center_point = calloc(1, sizeof(vector_t));
  center_point->x = 0;
  center_point->y = 0;
  list_add(pacman_shape, center_point);

  body_t *pacman = body_init(pacman_shape, DUMMY_MASS, PACMAN_COLOR);

  body_set_centroid(pacman, initial_pos);

  return pacman;
}

/**
 * Generates a pellet at a random location on the screen.
 *
 * @param scene pointer to the current scene of the program
 * @return pointer that points to the pellet
 */
body_t *make_pellet(scene_t *scene) {
  list_t *pellet_shape = list_init(PELLET_NUM_POINTS, free);

  double curr_angle = 0.0;
  double dt = 360 / PELLET_NUM_POINTS; // change in angle in each point

  for (size_t i = 0; i < PELLET_NUM_POINTS; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = PELLET_RADIUS * cos(deg_to_rad(curr_angle));
    new_point->y = PELLET_RADIUS * sin(deg_to_rad(curr_angle));
    list_add(pellet_shape, new_point);

    curr_angle += dt;
  }

  body_t *pellet = body_init(pellet_shape, DUMMY_MASS, PELLET_COLOR);

  bool valid_pos = false;

  // Randomize position
  vector_t pos = (vector_t){
      rand() % (int)(WINDOW.x - 2 * PELLET_RADIUS) + (int)(PELLET_RADIUS),
      rand() % (int)(WINDOW.y - 2 * PELLET_RADIUS) + (int)(PELLET_RADIUS)};
  // Keep generating new positions until the pellet does not collide with other
  // pellets
  while (!valid_pos && scene_bodies(scene) > 2) {
    pos = (vector_t){
        rand() % (int)(WINDOW.x - 2 * PELLET_RADIUS) + (int)(PELLET_RADIUS),
        rand() % (int)(WINDOW.y - 2 * PELLET_RADIUS) + (int)(PELLET_RADIUS)};
    for (size_t i = 1; i < scene_bodies(scene); i++) {
      body_t *pellet_i = scene_get_body(scene, i);
      vector_t pellet_i_centroid = body_get_centroid(pellet_i);
      // Distance between the center of the two pellets
      double d = sqrt(pow(pos.x - pellet_i_centroid.x, 2) +
                      pow(pos.y - pellet_i_centroid.y, 2));
      if (d > 2 * PELLET_RADIUS) {
        valid_pos = true;
      } else {
        break;
      }
    }
  }

  body_set_centroid(pellet, pos);

  return pellet;
}

/**
 * Make the pellet disappear once the pacman is fully covering a
 * pellet.
 *
 * @param scene pointer to the current scene of the program
 */
void check_if_eating_pellet(scene_t *scene) {
  vector_t pacman_centroid = body_get_centroid(scene_get_body(scene, 0));

  // Loop through each pellet
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    vector_t pellet_centroid = body_get_centroid(scene_get_body(scene, i));
    // Distance between the center of the pacman and pellet
    double d = sqrt(pow(pacman_centroid.x - pellet_centroid.x, 2) +
                    pow(pacman_centroid.y - pellet_centroid.y, 2));
    // Remove if pellet touches pacman
    if (d < PACMAN_RADIUS) {
      scene_remove_body(scene, i);
    }
  }
}

/**
 * Checks and translates pacman's location when it goes off-screen.
 *
 * @param scene pointer to the current scene
 */
void check_wrap_around(scene_t *scene) {
  body_t *pacman = scene_get_body(scene, 0);
  vector_t pacman_centroid = body_get_centroid(pacman);
  vector_t new_centroid = pacman_centroid;
  // Check if the pacman's centroid is outside of the window
  if (pacman_centroid.x <= -PACMAN_RADIUS) {
    new_centroid = (vector_t){WINDOW.x + PACMAN_RADIUS - 1, pacman_centroid.y};
  } else if (pacman_centroid.x >= WINDOW.x + PACMAN_RADIUS) {
    new_centroid = (vector_t){-PACMAN_RADIUS + 1, pacman_centroid.y};
  } else if (pacman_centroid.y <= -PACMAN_RADIUS) {
    new_centroid = (vector_t){pacman_centroid.x, WINDOW.y + PACMAN_RADIUS - 1};
  } else if (pacman_centroid.y >= WINDOW.y + PACMAN_RADIUS) {
    new_centroid = (vector_t){pacman_centroid.x, -PACMAN_RADIUS + 1};
  }
  body_set_centroid(pacman, new_centroid);
}

/**
 * Calculate the new velocity vector based on the direction
 * and speed of the pacman.
 *
 * @param direction current direction of pacman
 * @param speed current speed of pacman
 * @return vector containing the new velocity
 */
vector_t calculate_new_vel(double direction, double speed) {
  vector_t new_vel = {0.0, 0.0};
  if (direction == 0) {
    new_vel.x = speed;
    new_vel.y = 0;
  } else if (direction == 90) {
    new_vel.x = 0;
    new_vel.y = speed;
  } else if (direction == 180) {
    new_vel.x = -speed;
    new_vel.y = 0;
  } else if (direction == -90) {
    new_vel.x = 0;
    new_vel.y = -speed;
  }
  return new_vel;
}

/**
 * Make the pacman move and rotate according to keyboard event.
 *
 * @param state pointer to the current state of the program
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
void on_key(state_t *state, char key, key_event_type_t type, double held_time) {
  body_t *pacman = scene_get_body(state->scene, 0);
  vector_t old_vel = body_get_velocity(pacman);
  double pacman_direction = state->pacman_direction;
  double new_speed = ZERO_ACCEL_SPEED;
  vector_t new_vel = old_vel;
  if (type == KEY_PRESSED) {
    // Rotate pacman
    if (key <= 4) { // make sure only arrow keys are registered
      double new_pacman_direction = 90 * (-key + 3);
      double rotation = new_pacman_direction - pacman_direction;
      state->pacman_direction = new_pacman_direction;
      body_set_rotation(pacman, deg_to_rad(rotation));
    }

    // Accelerate pacman
    new_speed += ACCELERATION * held_time;
    // Ensure pacman is not too fast
    if (new_speed > TERMINAL_SPEED) {
      new_speed = TERMINAL_SPEED;
    }

    new_vel = calculate_new_vel(state->pacman_direction, new_speed);

    // Runs only at the beginning of program when pacman stationary
    if (old_vel.x == 0 && old_vel.y == 0) {
      new_vel = calculate_new_vel(state->pacman_direction, ZERO_ACCEL_SPEED);
    }
  } else {
    // Make pacman continue moving at constant speed if not accelerating
    new_vel = calculate_new_vel(state->pacman_direction, ZERO_ACCEL_SPEED);
  }
  body_set_velocity(pacman, new_vel);
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

  state_t *state = calloc(1, sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;

  scene_add_body(state->scene,
                 make_pacman(PACMAN_INITIAL_POS, PACMAN_INITIAL_MOUTH_ANGLE));

  srand(time(NULL)); // randomize seed

  // Generate initial pellets
  for (size_t i = 0; i < PELLET_INITIAL_COUNT; i++) {
    scene_add_body(state->scene, make_pellet(state->scene));
  }

  sdl_on_key(on_key);

  state->pacman_direction = PACMAN_INITIAL_DIRECTION;

  return state;
}

/**
 * Update screen at each frame.
 *
 * @param state a pointer to the current state of the program
 * @return a pointer to the current state of the program
 */
void emscripten_main(state_t *state) {
  double dt = time_since_last_tick();

  scene_t *scene = state->scene;

  check_wrap_around(scene);
  check_if_eating_pellet(scene);
  scene_tick(scene, dt);

  // Spawn new pellet
  state->time_since_last_pellet += dt;
  if (state->time_since_last_pellet >= 3) {
    scene_add_body(scene, make_pellet(scene));
    state->time_since_last_pellet = 0;
  }

  sdl_render_scene(scene);
}

/**
 * Free memory allocated to the state and contents within.
 *
 * @param state a pointer to the currentd state
 */
void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}