#include "../include/body.h"
#include "../include/collision.h"
#include "../include/forces.h"
#include "../include/scene.h"
#include "../include/sdl_wrapper.h"
#include "../include/shapes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Window constants
const vector_t WINDOW = (vector_t){.x = 800, .y = 600};
const vector_t CENTER = (vector_t){.x = 400, .y = 300};

const vector_t SPAWN_WINDOW = (vector_t){.x = 1000.0, .y = 200.0};
const vector_t POWER_UP_SPAWN_WINDOW = (vector_t){.x = 800.0, .y = 200};

const vector_t ARRAY_BRICKS = {10, 3}; // x: # cols, y: # rows of initial bricks
const double BRICK_HEIGHT = 25;
const double BRICK_MASS = INFINITY;
const double BRICK_SPACE = 5;

const double TOTAL_CIRCLE_ANGLE = 360;
const double BALL_RADIUS = 10;
const double BALL_MASS = 10;
const rgb_color_t BALL_COLOR = {1, 0, 0};
const vector_t BALL_INITIAL_POS = {400, 70};
const vector_t BALL_INITIAL_VEL = {200, 150};
const double BALL_VEL_INCREASE_FACTOR = 1.05;

const vector_t PADDLE_MOVE_DIST = {30, 0};
const double PADDLE_MASS = INFINITY;
const rgb_color_t PADDLE_COLOR = {1, 0, 0};
const vector_t PADDLE_INITIAL_POS = {400, 25};
const double PADDLE_ELASTICITY = 1;

const double WALL_ELASTICITY = 1;
const double BRICK_ELASTICITY = 1;

typedef enum {
  BALL_DESTRUCTIVE,
  BALL_PHYSICS,
  PADDLE,
  BRICK,
  WALL,
  RESET,
  POWER_UP
} body_type_t;

typedef struct state {
  scene_t *scene;
} state_t;

/**
 * Make an info field based on body type.
 *
 * @param type type of body
 * @return pointer to an info
 */
body_type_t *make_type_info(body_type_t type) {
  body_type_t *info = malloc(sizeof(*info));
  *info = type;
  return info;
}

/**
 * Get the info associated with the inputted body.
 *
 * @param body pointer to the body
 * @return body type
 */
body_type_t get_type(body_t *body) {
  return *(body_type_t *)body_get_info(body);
}

/**
 * Convert an angle from degrees to radians.
 *
 * @param deg angle in degrees
 * @return angle in radians
 */
double deg_to_rad(double deg) { return deg * M_PI / 180; }

/**
 * Create a brick shaped object to be drawn to the screen.
 *
 * @param initial_pos initial position for the brick
 * @param brick_color color of the brick
 * @return a pointer to the body at the brickcolor
 */

body_t *make_brick_body(vector_t initial_pos, rgb_color_t brick_color) {
  double brick_width = WINDOW.x / ARRAY_BRICKS.x - BRICK_SPACE;

  // Initialize the brick shape
  list_t *brick_shape = make_rect_shape(brick_width, BRICK_HEIGHT);

  // Initialize body
  body_t *brick_body = body_init_with_info(brick_shape, BRICK_MASS, brick_color,
                                           make_type_info(BRICK), free);
  body_set_centroid(brick_body, initial_pos);

  return brick_body;
}

/**
 * Create a body for the paddle.
 *
 * @return a pointer to the body of the paddle
 */

body_t *make_paddle_body() {
  double brick_width = WINDOW.x / ARRAY_BRICKS.x - BRICK_SPACE;

  // Initialize the brick shape
  list_t *brick_shape = make_rect_shape(brick_width, BRICK_HEIGHT);

  // Initialize body
  body_t *paddle_body = body_init_with_info(
      brick_shape, PADDLE_MASS, PADDLE_COLOR, make_type_info(PADDLE), free);
  body_set_centroid(paddle_body, PADDLE_INITIAL_POS);

  return paddle_body;
}

/**
 * Generates a ball body at the given initial pos.
 *
 * @param initial_pos initial position of the ball
 * @param initial_vel initial velocity of the ball
 * @param ball_type the type of ball (Destructive or Physics)
 * @return pointer to the body of the ball
 */
body_t *make_ball_body(vector_t initial_pos, vector_t initial_vel,
                       body_type_t ball_type) {
  // Initialize circle shape
  list_t *ball_shape = list_init(TOTAL_CIRCLE_ANGLE, free);

  double curr_angle = 0.0;
  double dt = 1; // change in angle in each point

  for (size_t i = 0; i < TOTAL_CIRCLE_ANGLE; i++) {
    vector_t *new_point = calloc(1, sizeof(vector_t));
    new_point->x = BALL_RADIUS * cos(deg_to_rad(curr_angle));
    new_point->y = BALL_RADIUS * sin(deg_to_rad(curr_angle));
    list_add(ball_shape, new_point);

    curr_angle += dt;
  }

  // Initialize ball body
  body_t *ball_body = body_init_with_info(ball_shape, BALL_MASS, BALL_COLOR,
                                          make_type_info(ball_type), free);
  body_set_centroid(ball_body, initial_pos);
  body_set_velocity(ball_body, initial_vel);

  return ball_body;
}

/**
 * Ensure paddle is still in screen boundaries.
 *
 * @param scene pointer to the current scene of the game.
 */
void ensure_paddle_on_screen(scene_t *scene) {
  // double paddle_height = BRICK_HEIGHT;
  double paddle_width = WINDOW.x / ARRAY_BRICKS.x - BRICK_SPACE;

  // Check the first body in scene is the player ship
  body_t *paddle_body = scene_get_body(scene, 0);
  if (get_type(paddle_body) == PADDLE) {
    // Ensure player not out of left side of screen
    if (body_get_centroid(paddle_body).x - paddle_width / 2 < 0) {
      body_set_centroid(
          paddle_body,
          (vector_t){paddle_width / 2, body_get_centroid(paddle_body).y});
    }
    // Ensure player not out of right side of screen
    if (body_get_centroid(paddle_body).x + paddle_width / 2 > WINDOW.x) {
      body_set_centroid(paddle_body,
                        (vector_t){WINDOW.x - paddle_width / 2,
                                   body_get_centroid(paddle_body).y});
    }
  }
}

/**
 * Adds the walls to the scene.
 *
 * @param scene a pointer to the current scene of the game
 * */
void add_walls(scene_t *scene) {
  // Add walls
  list_t *top = make_rect_shape(WINDOW.x, BRICK_SPACE);
  list_t *bottom = make_rect_shape(WINDOW.x, BRICK_SPACE);
  list_t *left = make_rect_shape(BRICK_SPACE, WINDOW.y);
  list_t *right = make_rect_shape(BRICK_SPACE, WINDOW.y);

  body_t *top_body = body_init_with_info(
      top, INFINITY, (rgb_color_t){0.0, 0.0, 0.0}, make_type_info(WALL), free);
  body_set_centroid(top_body,
                    (vector_t){CENTER.x, WINDOW.y + BRICK_SPACE / 2.0});
  scene_add_body(scene, top_body);

  body_t *left_body = body_init_with_info(
      left, INFINITY, (rgb_color_t){0.0, 0.0, 0.0}, make_type_info(WALL), free);
  body_set_centroid(left_body, (vector_t){-BRICK_SPACE / 2.0, CENTER.y});
  scene_add_body(scene, left_body);

  body_t *right_body =
      body_init_with_info(right, INFINITY, (rgb_color_t){0.0, 0.0, 0.0},
                          make_type_info(WALL), free);
  body_set_centroid(right_body,
                    (vector_t){WINDOW.x + BRICK_SPACE / 2.0, CENTER.y});
  scene_add_body(scene, right_body);

  // Ground is special; resets game when ball touches
  body_t *bottom_body =
      body_init_with_info(bottom, INFINITY, (rgb_color_t){0.0, 0.0, 0.0},
                          make_type_info(RESET), free);
  body_set_centroid(bottom_body, (vector_t){CENTER.x, -BRICK_SPACE / 2.0});
  scene_add_body(scene, bottom_body);
}

/**
 * Add paddle to the scene.
 *
 * @param scene pointer to the current scene of the game
 */
void add_paddle(scene_t *scene) { scene_add_body(scene, make_paddle_body()); }

/**
 * Add the ball's body and appropriate collision types to the sceen
 *
 * @param scene a pointer to the current scene of the game
 * @param initial_pos initial position of the ball
 * @param initial_vel initial velocity of the ball
 * @param ball_type the type of ball (Destructive or Physics)
 */
void add_ball(scene_t *scene, vector_t initial_pos, vector_t initial_vel,
              body_type_t ball_type) {
  body_t *ball_body = make_ball_body(initial_pos, initial_vel, ball_type);

  size_t body_count = scene_bodies(scene);
  scene_add_body(scene, ball_body);

  // Add force creators with other bodies
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    switch (get_type(body)) {
    case WALL:
      create_physics_collision(scene, WALL_ELASTICITY, ball_body, body);
      break;
    case PADDLE:
      create_physics_collision(scene, PADDLE_ELASTICITY, ball_body, body);
      break;
    case BRICK:
      if (ball_type == BALL_DESTRUCTIVE) {
        create_destructive_collision(scene, ball_body, body);
      } else if (ball_type == BALL_PHYSICS) {
        create_physics_collision(scene, BRICK_ELASTICITY, ball_body, body);
      }
      break;
    case RESET:
      create_destructive_collision(scene, ball_body, body);
      break;
    }
  }
}

/**
 * Add the grid of bricks to the scene.
 *
 * @param scene a pointer to the current scene of the game
 */

void add_bricks(scene_t *scene) {
  vector_t spawn_window = {WINDOW.x / ARRAY_BRICKS.x,
                           BRICK_HEIGHT + BRICK_SPACE};
  double dh = TOTAL_CIRCLE_ANGLE / ARRAY_BRICKS.x;
  for (size_t c = 0; c < ARRAY_BRICKS.x; c++) {
    double h = c * dh;
    rgb_color_t color = hsv_to_rgb(h, 1, 1);
    for (size_t r = 0; r < ARRAY_BRICKS.y; r++) {
      vector_t initial_pos = {c * spawn_window.x + spawn_window.x / 2,
                              WINDOW.y -
                                  (r * spawn_window.y + spawn_window.y / 2)};
      scene_add_body(scene, make_brick_body(initial_pos, color));
    }
  }
}

/**
 * Replace the ball that performs destructive collisions with bricks
 * with a new ball.
 *
 * @param scene a pointer to the current scene of the game
 */
void replace_destructive_ball(scene_t *scene) {
  bool destructive_ball_on_screen = false;
  body_t *physics_ball;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (get_type(body) == BALL_DESTRUCTIVE) {
      destructive_ball_on_screen = true;
    } else if (get_type(body) == BALL_PHYSICS) {
      physics_ball = body;
    }
  }

  // Reset the destructive ball and speeds up the ball's velocity
  // when it breaks a brick (SPECIAL FEATURE)
  if (!destructive_ball_on_screen) {
    body_set_velocity(physics_ball,
                      vec_multiply(BALL_VEL_INCREASE_FACTOR,
                                   body_get_velocity(physics_ball)));
    add_ball(scene, body_get_centroid(physics_ball),
             body_get_velocity(physics_ball), BALL_DESTRUCTIVE);
  }
}

/**
 * Make the paddle move left or right.
 *
 * @param state pointer to the current state of the program
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
void on_key(state_t *state, char key, key_event_type_t type, double held_time) {
  if (type == KEY_PRESSED) {
    body_t *paddle_body = scene_get_body(state->scene, 0);

    if (key == LEFT_ARROW) {
      vector_t new_centroid =
          vec_add(body_get_centroid(paddle_body), vec_negate(PADDLE_MOVE_DIST));
      body_set_centroid(paddle_body, new_centroid);
    } else if (key == RIGHT_ARROW) {
      vector_t new_centroid =
          vec_add(body_get_centroid(paddle_body), PADDLE_MOVE_DIST);
      body_set_centroid(paddle_body, new_centroid);
    }
  }
}

/**
 * Restart the game.
 *
 * @param state the current state of the game
 */
void make_new_game(state_t *state) {
  scene_free(state->scene);

  // Initialize a new scene
  scene_t *new_scene = scene_init();
  state->scene = new_scene;

  add_paddle(new_scene);
  add_bricks(new_scene);
  add_walls(new_scene);

  add_ball(new_scene, BALL_INITIAL_POS, BALL_INITIAL_VEL, BALL_DESTRUCTIVE);
  add_ball(new_scene, BALL_INITIAL_POS, BALL_INITIAL_VEL, BALL_PHYSICS);

  sdl_on_key(on_key);
}

/**
 * Check if the game has reaeched either of its end game conditions.
 *  1) If ball hits bottom of screen.
 *  2) If no more bricks remaining.
 *
 * @param scene a pointer to the current scene of the game
 * @return boolean representing whether the game has eneded
 */
bool check_game_ended(scene_t *scene) {
  bool ball_physics_on_screen = false;
  size_t num_bricks = 0;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (get_type(scene_get_body(scene, i)) == BALL_PHYSICS) {
      ball_physics_on_screen = true;
    }
    if (get_type(scene_get_body(scene, i)) == BRICK) {
      num_bricks++;
    }
  }
  return !ball_physics_on_screen || (num_bricks == 0);
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

  add_paddle(scene);
  add_bricks(scene);
  add_walls(scene);

  add_ball(scene, BALL_INITIAL_POS, BALL_INITIAL_VEL, BALL_DESTRUCTIVE);
  add_ball(scene, BALL_INITIAL_POS, BALL_INITIAL_VEL, BALL_PHYSICS);

  sdl_on_key(on_key);

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

  ensure_paddle_on_screen(scene);

  replace_destructive_ball(scene);

  scene_tick(scene, dt);
  if (check_game_ended(scene)) {
    make_new_game(state);
  }
  sdl_render_scene(state->scene);
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
