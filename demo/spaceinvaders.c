#include "../include/body.h"
#include "../include/collision.h"
#include "../include/forces.h"
#include "../include/sdl_wrapper.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 800};
const vector_t CENTER = (vector_t){.x = 500, .y = 400};

const vector_t ARRAY_ALIEN_SHIPS = {
    8.0, 3.0}; // x: # cols, y: # rows of initial alien ships
const double ALIEN_SHIP_RADIUS = 40;
const vector_t ALIEN_SHIP_INITIAL_VEL = {50, 0};
const vector_t PLAYER_SHIP_DIMS = {80.0, 40.0}; // x: width, y: height
const vector_t PLAYER_SHIP_INITIAL_POS = {500, 21};
const vector_t BULLET_DIMS = {10, 30}; // x: width, y: height
const vector_t BULLET_VEL = {0, 1000};
const rgb_color_t ALIEN_COLOR = {.5, .5, .5}; // for ship and projectile
const rgb_color_t PLAYER_COLOR = {0, 1, 0};   // for ship and projectile
const size_t NUM_ALIEN_SHIP_POINTS = 181;
const double TOTAL_CIRCLE_ANGLE = 360;
const double DUMMY_MASS = 1.0;
const double ALN_SHP_SPWN_WNDW_HT_CONST = 1.7;
const double ALIEN_BULLET_SPAWN_PERIOD = 3;
const vector_t MOVE_DIST = {30, 0};

/**
 * A struct to represent the current state of the program.
 */
typedef struct state {
  scene_t *scene;
  double time_since_last_bullet;
} state_t;

/**
 * Convert an angle from degrees to radians.
 *
 * @param deg angle in degrees
 * @return angle in radians
 */
double deg_to_rad(double deg) { return deg * M_PI / 180; }

/**
 * Initialize an alien ship body at the given position.
 *
 * @param initial_pos initial position of body
 * @return alien ship body
 */
body_t *make_alien_ship_body(vector_t initial_pos) {
  // Initialize the ship shape
  list_t *ship_shape = list_init(NUM_ALIEN_SHIP_POINTS, free);

  double curr_angle = 0;
  double dt = 1; // change in angle in each point

  for (size_t i = 0; i < NUM_ALIEN_SHIP_POINTS - 1; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = ALIEN_SHIP_RADIUS * cos(deg_to_rad(curr_angle));
    new_point->y = ALIEN_SHIP_RADIUS * sin(deg_to_rad(curr_angle));
    list_add(ship_shape, new_point);

    curr_angle += dt;
  }

  // Add point to make triangle below semi-circle
  vector_t *bottom_point = calloc(1, sizeof(vector_t));
  bottom_point->x = 0;
  bottom_point->y = -ALIEN_SHIP_RADIUS / 2;
  list_add(ship_shape, bottom_point);

  // Initialize body
  char *info = "AS"; // Alien Ship
  body_t *ship_body =
      body_init_with_info(ship_shape, DUMMY_MASS, ALIEN_COLOR, info, free);
  body_set_centroid(ship_body, initial_pos);
  body_set_velocity(ship_body, ALIEN_SHIP_INITIAL_VEL);

  return ship_body;
}

/**
 * Initialize a player ship body.
 *
 * @return a pointer to the player ship's body
 */
body_t *make_player_ship_body() {
  // Initialize the ship shape
  list_t *ship_shape = list_init(TOTAL_CIRCLE_ANGLE + 1, free);

  double curr_angle = 0;
  double dt = 1; // change in angle in each point

  for (size_t i = 0; i < TOTAL_CIRCLE_ANGLE; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = PLAYER_SHIP_DIMS.x / 2 * cos(deg_to_rad(curr_angle));
    new_point->y = PLAYER_SHIP_DIMS.y / 2 * sin(deg_to_rad(curr_angle));
    list_add(ship_shape, new_point);

    curr_angle += dt;
  }

  // Initialize body
  char *info = "PS"; // Player Ship
  body_t *ship_body =
      body_init_with_info(ship_shape, DUMMY_MASS, PLAYER_COLOR, info, free);
  body_set_centroid(ship_body, PLAYER_SHIP_INITIAL_POS);

  return ship_body;
}

/**
 * Initialize a bullet body based on the type of body passed in.
 *
 * @return a pointer to the bullet's body
 */
body_t *make_bullet_body(body_t *body) {
  // Initialize the bullet shape
  list_t *bullet_shape = list_init(4, free);
  vector_t *v1 = calloc(1, sizeof(vector_t));
  vector_t *v2 = calloc(1, sizeof(vector_t));
  vector_t *v3 = calloc(1, sizeof(vector_t));
  vector_t *v4 = calloc(1, sizeof(vector_t));
  *v1 = (vector_t){BULLET_DIMS.x / 2, BULLET_DIMS.y / 2};
  *v2 = (vector_t){-BULLET_DIMS.x / 2, BULLET_DIMS.y / 2};
  *v3 = (vector_t){-BULLET_DIMS.x / 2, -BULLET_DIMS.y / 2};
  *v4 = (vector_t){BULLET_DIMS.x / 2, -BULLET_DIMS.y / 2};
  list_add(bullet_shape, v1);
  list_add(bullet_shape, v2);
  list_add(bullet_shape, v3);
  list_add(bullet_shape, v4);

  // Initialize bullet info and bullet color
  char *bullet_info = calloc(3, sizeof(char));
  char *body_info = body_get_info(body);
  rgb_color_t color = {0, 0, 0};
  if (strcmp(body_info, "AS") == 0) {
    strcpy(bullet_info, "AB");
    color = ALIEN_COLOR;
  } else if (strcmp(body_info, "PS") == 0) {
    strcpy(bullet_info, "PB");
    color = PLAYER_COLOR;
  }

  // Initialize bullet body
  body_t *bullet_body =
      body_init_with_info(bullet_shape, DUMMY_MASS, color, bullet_info, free);
  body_set_centroid(bullet_body, body_get_centroid(body));

  // Set velocity based on type of body shooting bullet from
  if (strcmp(body_info, "AS") == 0) {
    body_set_velocity(bullet_body, vec_negate(BULLET_VEL));
  } else if (strcmp(body_info, "PS") == 0) {
    body_set_velocity(bullet_body, BULLET_VEL);
  }

  return bullet_body;
}

/**
 * Wrap an alien ship down a level.
 *
 * @param alien_ship_body pointer to alien ship
 */
void wrap_alien_ship(body_t *alien_ship_body) {
  // Left most point
  list_t *alien_ship_shape = body_get_shape(alien_ship_body);
  vector_t left_point =
      *(vector_t *)list_get(alien_ship_shape, NUM_ALIEN_SHIP_POINTS - 2);
  // Right most point
  vector_t right_point = *(vector_t *)list_get(alien_ship_shape, 0);

  // Check if out of left side of screen.
  if (left_point.x <= 0) {
    vector_t new_pos = {ALIEN_SHIP_RADIUS + 1,
                        body_get_centroid(alien_ship_body).y -
                            3 * ALN_SHP_SPWN_WNDW_HT_CONST * ALIEN_SHIP_RADIUS};
    body_set_centroid(alien_ship_body, new_pos);
    body_set_velocity(alien_ship_body,
                      vec_negate(body_get_velocity(alien_ship_body)));
  }
  // Check if out of right side of screen.
  else if (right_point.x >= WINDOW.x) {
    vector_t new_pos = {WINDOW.x - ALIEN_SHIP_RADIUS - 1,
                        body_get_centroid(alien_ship_body).y -
                            3 * ALN_SHP_SPWN_WNDW_HT_CONST * ALIEN_SHIP_RADIUS};
    body_set_centroid(alien_ship_body, new_pos);
    body_set_velocity(alien_ship_body,
                      vec_negate(body_get_velocity(alien_ship_body)));
  }

  list_free(alien_ship_shape);
}

/**
 * Ensure player is still in screen boundaries.
 *
 * @param scene pointer to the current scene of the game.
 */
void ensure_player_on_screen(scene_t *scene) {
  // Check the first body in scene is the player ship
  body_t *player_body = scene_get_body(scene, 0);
  if (strcmp((char *)body_get_info(player_body), "PS") == 0) {
    // Ensure player not out of left side of screen
    if (body_get_centroid(player_body).x - PLAYER_SHIP_DIMS.x / 2 < 0) {
      body_set_centroid(
          player_body,
          (vector_t){PLAYER_SHIP_DIMS.x / 2, body_get_centroid(player_body).y});
    }
    // Ensure player not out of right side of screen
    if (body_get_centroid(player_body).x + PLAYER_SHIP_DIMS.x / 2 > WINDOW.x) {
      body_set_centroid(player_body,
                        (vector_t){WINDOW.x - PLAYER_SHIP_DIMS.x / 2,
                                   body_get_centroid(player_body).y});
    }
  }
}

/**
 * Checks if player's previous bullet is off screen.
 *
 * @param scene pointer to the current scene of the game
 * @return whether the player bullet is on screen or not
 */
bool is_player_bullet_on_screen(scene_t *scene) {
  // Loop through all bodies and check if a Player Bullet (PB) body
  // is in the scene and that it is in the WINDOW
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (strcmp((char *)body_get_info(body), "PB") == 0) {
      if ((0 <= body_get_centroid(body).x &&
           body_get_centroid(body).x <= WINDOW.x) &&
          (0 <= body_get_centroid(body).y &&
           body_get_centroid(body).y <= WINDOW.y)) {
        return true;
      }
    }
  }
  return false;
}

/**
 * Shoot a bullet from the inputted body.
 *
 * @param scene pointer to the current scene of the game
 * @param body pointer to the body to shoot bullet from
 */
void shoot_bullet(scene_t *scene, body_t *body) {
  char *body_info = body_get_info(body);
  body_t *bullet_body = make_bullet_body(body);
  scene_add_body(scene, bullet_body);

  // Check if shooting from alien ship body
  if (strcmp(body_info, "AS") == 0) {
    // Create destructive collision with bullet body and player body
    body_t *player_body = scene_get_body(scene, 0);
    if (strcmp((char *)body_get_info(player_body), "PS") == 0) {
      create_destructive_collision(scene, player_body, bullet_body);
    }
  }
  // Check if shooting from player body
  else if (strcmp(body_info, "PS") == 0) {
    // Create destructive collisions with bullet body and every alien ship body
    for (size_t i = 0; i < scene_bodies(scene); i++) {
      body_t *other_body = scene_get_body(scene, i);
      if (strcmp((char *)body_get_info(other_body), "AS") == 0) {
        create_destructive_collision(scene, other_body, bullet_body);
      }
    }
  }
}

/**
 * Remove all bullets off screen that were previously shot by either player or
 * aliens.
 *
 * @param pointer to the current scene of the game.
 */
void remove_missed_bullets(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    char *info = body_get_info(body);
    if (strcmp(info, "PB") == 0 || strcmp(info, "AB") == 0) {
      if ((0 > body_get_centroid(body).x ||
           body_get_centroid(body).x > WINDOW.x) ||
          (0 > body_get_centroid(body).y ||
           body_get_centroid(body).y > WINDOW.y)) {
        scene_remove_body(scene, i);
      }
    }
  }
}

/**
 * Checks to see if any of the 3 game ending conditions have been met.
 *
 * @param scene pointer to the current scene of the game.
 */
void check_end_game(scene_t *scene) {
  // Check player still on screen
  if (strcmp((char *)body_get_info(scene_get_body(scene, 0)), "PS") != 0) {
    exit(0);
  }

  // Check if all aliens dead
  if (strcmp((char *)body_get_info(scene_get_body(scene, 1)), "AS") != 0) {
    exit(0);
  }

  // Check if any of the alien ships have reached ground/player level
  body_t *player_body = scene_get_body(scene, 0);
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    body_t *other_body = scene_get_body(scene, i);
    if (strcmp((char *)body_get_info(other_body), "AS") == 0) {
      if (body_get_centroid(other_body).y <= body_get_centroid(player_body).y) {
        exit(0);
      }
    }
  }
}

/**
 * Make the player ship move left or right and shoot.
 *
 * @param state pointer to the current state of the program
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
void on_key(state_t *state, char key, key_event_type_t type, double held_time) {
  if (type == KEY_PRESSED) {
    // Check if player body is still in the scene
    body_t *player_body = scene_get_body(state->scene, 0);
    if (strcmp((char *)body_get_info(player_body), "PS") == 0) {
      // Move player left
      if (key == LEFT_ARROW) {
        vector_t new_centroid =
            vec_add(body_get_centroid(player_body), vec_negate(MOVE_DIST));
        body_set_centroid(player_body, new_centroid);
      }
      // Move player right
      else if (key == RIGHT_ARROW) {
        vector_t new_centroid =
            vec_add(body_get_centroid(player_body), MOVE_DIST);
        body_set_centroid(player_body, new_centroid);
      }
      // Shoot bullet from player
      if (key == SPACE && !is_player_bullet_on_screen(state->scene)) {
        shoot_bullet(state->scene, player_body);
      }
    }
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

  state_t *state = calloc(1, sizeof(state_t));
  scene_t *scene = scene_init();
  state->scene = scene;
  state->time_since_last_bullet = 0;

  scene_add_body(scene, make_player_ship_body());

  // Size of window in which alien ships can spawn in
  vector_t alien_ships_spawning_window = {WINDOW.x / ARRAY_ALIEN_SHIPS.x,
                                          ALN_SHP_SPWN_WNDW_HT_CONST *
                                              ALIEN_SHIP_RADIUS};
  // Add alien ship bodies
  for (size_t r = 0; r < ARRAY_ALIEN_SHIPS.x; r++) {
    for (size_t c = 0; c < ARRAY_ALIEN_SHIPS.y; c++) {
      vector_t initial_pos = {r * alien_ships_spawning_window.x +
                                  alien_ships_spawning_window.x / 2,
                              WINDOW.y - (c * alien_ships_spawning_window.y +
                                          alien_ships_spawning_window.y / 2)};
      scene_add_body(scene, make_alien_ship_body(initial_pos));
    }
  }

  sdl_on_key(on_key);

  srand(time(NULL)); // randomize seed

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

  scene_t *scene = state->scene;

  // Shoot bullet from random alien ship
  state->time_since_last_bullet += dt;
  if (state->time_since_last_bullet >= ALIEN_BULLET_SPAWN_PERIOD) {
    int rand_idx = rand() % scene_bodies(scene);
    body_t *body = scene_get_body(scene, rand_idx);
    char *info = body_get_info(body);
    while (strcmp(info, "AS") != 0) {
      rand_idx = rand() % scene_bodies(scene);
      body = scene_get_body(scene, rand_idx);
      info = body_get_info(body);
    }
    shoot_bullet(scene, body);
    state->time_since_last_bullet = 0;
  }

  ensure_player_on_screen(scene);

  check_end_game(scene);

  // Wrap alien ships, if needed
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (strcmp((char *)body_get_info(body), "AS") == 0) {
      wrap_alien_ship(body);
    }
  }

  remove_missed_bullets(scene);

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