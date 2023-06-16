#include "../include/body_type.h"
#include "../include/button.h"
#include "../include/collision.h"
#include "../include/connection.h"
#include "../include/forces.h"
#include "../include/platform.h"
#include "../include/polygon.h"
#include "../include/portal.h"
#include "../include/scene.h"
#include "../include/sdl_wrapper.h"
#include "../include/shapes.h"
#include <emscripten.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// Window constants
const vector_t WINDOW = (vector_t){.x = 1024, .y = 704};
const vector_t CENTER = (vector_t){.x = 512, .y = 352};

// Level and scene constants
const size_t NUM_LEVELS = 6;
const size_t START_SCREEN_IDX = 6;
const size_t GAME_WON_SCREEN_IDX = 7;
const size_t LEVEL_SCREEN_IDX = 8;
const size_t RULES_SCREEN_IDX = 9;

// Wall constants
const double WALL_THICKNESS = 64;
const rgb_color_t WALL_COLOR = {0.75, 0.75, 0.75};
const double WALL_ELASTICITY_PLAYER = 0.1;
const double WALL_ELASTICITY_BOX = 0.1;

// Surface constants
const rgb_color_t STANDING_SURFACE_COLOR = {.6, .6, .6};

const rgb_color_t PORTAL_SURFACE_COLOR = {.4, .2, 0};

const double JUMPABLE_ELASTICITY = 0.1;

// Player constants
const vector_t PLAYER_DIMS = {32, 64}; // width, height
const double PLAYER_MASS = 1000;
const rgb_color_t PLAYER_COLOR = {1, 0, 0};
const vector_t PLAYER_MOVE_DIST = {10, 0};
const double PLAYER_MOVE_FORCE_MAG = 4e7;
const double PLAYER_JUMP_SPEED = 250; // m/s
const double PLAYER_JUMP_THRESHOLD = 5;
const double PLAYER_MAX_SPEED = 1e3;

// Portal gun constants
const vector_t PORTAL_GUN_DIMS = {30, 10};
const double PORTAL_GUN_MASS = 1;
const rgb_color_t PORTAL_GUN_COLOR = {0, 0, 0};
const vector_t PORTAL_GUN_DISPLACEMENT = {20, 0};

// Portal projectile constants
const double PORTAL_PROJECTILE_RADIUS = 10;
const size_t PORTAL_PROJECTILE_NUM_POINTS = 20;
const double PORTAL_PROJECTILE_MASS = 1;
const double PORTAL_PROJECTILE_SPEED = 1000;

// Portal constants
const vector_t PORTAL_DIMS = {10, 96};
const rgb_color_t PORTAL1_COLOR = {0, 0, 1};
const rgb_color_t PORTAL2_COLOR = {1, .5, 0};
const double PORTAL_ADJUST_NUM = 5.0;

// Box constants
const vector_t BOX_DIMS = {32, 32};
const rgb_color_t BOX_COLOR = {.5, .5, .5};
const double BOX_MASS = 1000;
const vector_t BOX_DISPLACEMENT = {0, 0};
const double BOX_ELASTICITY = 0;

// Exit box/level constants
const rgb_color_t EXIT_BOX_COLOR = {0, 0, 0};
const vector_t EXIT_BOX_DIMS = {64, 64};

// Platform constants
const vector_t PLATFORM_DIMS = {128, 16};
const double PLATFORM_ELASTICITY = .3;
const rgb_color_t PLATFORM_COLOR = {.38, .38, .38};

// Button constants
const rgb_color_t BUTTON_COLOR = {0, .6, 0};
const rgb_color_t BUTTON_BASE_COLOR = {.12, .12, .12};
const vector_t BUTTON_DIMS = {76, 8};
const vector_t BUTTON_BASE_DIMS = {100, 11};
const double BUTTON_ELASTICITY = 0;

// Timer constants
const vector_t TIMER_DIMS = {192, 64};
const vector_t TIMER_POS = {128, 672};
const char *TIMER_FONT_PATH = "assets/fonts/Arial.ttf";
const rgb_color_t TIMER_COLOR = {0, 0, 0};
const rgb_color_t TIMER_BG_COLOR = {1, 1, 1};
const size_t TIMER_FONTSIZE = 40;

// Image paths
const char *START_SCREEN_IMG_PATH = "assets/images/start_screen.png";
const char *GAME_WON_IMG_PATH = "assets/images/game_won_screen.png";
const char *LEVEL_SCREEN_IMG_PATH = "assets/images/level_screen.png";
const char *RULES_SCREEN_IMG_PATH = "assets/images/rules_screen.png";
const char *BOX_IMG_PATH = "assets/images/box.png";
const char *PORTAL_1_IMG_PATH = "assets/images/portal_1.png";
const char *PORTAL_2_IMG_PATH = "assets/images/portal_2.png";
const char *PLAYER_RIGHT_IMG_PATH = "assets/images/player_right.png";
const char *PLAYER_LEFT_IMG_PATH = "assets/images/player_left.png";
const bool USE_PORTAL_IMAGES = false;
const char *PORTAL_GUN_SOUND_PATH = "assets/sounds/portal_gun.wav";
const char *BACKGROUND_MUSIC_FILE_PATH = "assets/sounds/background_music.wav";

// Gravitaion constants
// g: 983 m/s^2
const double G = 6.67E-9; // N m^2 / kg^2
const double M = 6E24;    // kg
const double R = 6.38E6;  // m

/**
 * A struct to represent the current state of the program.
 */
typedef struct state {
  list_t *scenes;

  size_t curr_level;

  body_t *player_body;
  body_t *exit_body;
  body_t *timer_body;
  body_t *portal_projectile_body;

  bool *is_jumping;
  bool *is_player_teleporting;
  bool *is_box_teleporting;
  bool is_portal_restricted;

  portal_t *portal1;
  portal_t *portal2;

  SDL_Texture *player_left_image;
  SDL_Texture *player_right_image;

  vector_t mouse_pos;

  connection_t *portal_gun_connection;
  list_t *box_connections;

  list_t *platforms;

  list_t *buttons;

  double timer;
  double last_time;
} state_t;

// -----------------------  HELPER FUNCTIONS  -----------------------

/**
 * Gets the current scene of a state based on the state's current level
 *
 * @param state a pointer to a state
 * @return a pointer to the state's current scene
 */
scene_t *get_curr_scene(state_t *state) {
  return list_get(state->scenes, state->curr_level);
}

void init_new_level(state_t *state);

/**
 * Checks if the current level has ended. If so, go to next level/screen.
 *
 * @param state a pointer to a state
 */
void check_end_level(state_t *state) {
  // Check if out of time, if so restart level
  if (state->timer <= 0) {
    init_new_level(state);
    return;
  }

  body_t *player_body = state->player_body;
  body_t *exit_body = state->exit_body;

  vector_t player_centroid = body_get_centroid(player_body);
  vector_t exit_centroid = body_get_centroid(exit_body);

  double max_distance = sqrt(pow(PLAYER_DIMS.x / 2 - EXIT_BOX_DIMS.x / 2, 2) +
                             pow(PLAYER_DIMS.y / 2 - EXIT_BOX_DIMS.y / 2, 2));
  double distance = sqrt(vec_dot(vec_subtract(player_centroid, exit_centroid),
                                 vec_subtract(player_centroid, exit_centroid)));

  // Reset level if fall out of screen
  if (player_centroid.y <= 0) {
    init_new_level(state);
  }

  // Check if inside exit box
  if (state->player_body != NULL && distance <= max_distance) {
    // Check if finished last level
    if (state->curr_level == NUM_LEVELS - 1) {
      state->curr_level = GAME_WON_SCREEN_IDX;
      init_new_level(state);
    } else {
      state->curr_level++;
      init_new_level(state);
    }
  }
}

/**
 * Calculate the direction of the vector from the positions
 * of the inputted body to a  mouse.
 *
 * @param mouse_pos a vector of the mouse position on the screen
 * @param body a pointer to the body find direction around
 */
double calculate_mouse_direction(vector_t mouse_pos, body_t *body) {
  vector_t body_centroid = body_get_centroid(body);
  vector_t direction_vec = vec_subtract(mouse_pos, body_centroid);

  return vec_direction_angle(direction_vec);
}

/**
 * Rotate the portal gun to the mouse direction.
 *
 * @param state a pointer to a state
 */
void rotate_portal_gun(state_t *state) {
  body_t *player_body = state->player_body;

  double portal_gun_direction =
      calculate_mouse_direction(state->mouse_pos, player_body);

  connection_set_rotation(state->portal_gun_connection, portal_gun_direction,
                          true);
}

/**
 * Displays the time left in the current level of a state.
 *
 * @param state a pointer to a state
 */
void display_timer(state_t *state) {
  if (trunc(state->timer) != state->last_time) {
    // Generate text texture
    char timer_text[50];
    sprintf(timer_text, "Time left: %d", (int)state->timer);
    SDL_Texture *timer_texture =
        sdl_load_text(timer_text, TIMER_COLOR, TIMER_FONT_PATH, TIMER_FONTSIZE);

    body_set_text(state->timer_body, timer_texture);
  }
}

/**
 * Resets a state's current level to it's initialized scene.
 *
 * @param state a pointer to a state
 */
void reset_level(state_t *state) {
  // Reset scene
  scene_free(get_curr_scene(state));

  // Reset buttons
  if (state->buttons) {
    list_free(state->buttons);
  }
  state->buttons = list_init(1, (free_func_t)button_free);

  // Reset portals
  if (state->portal1) {
    free(state->portal1);
  }
  if (state->portal2) {
    free(state->portal2);
  }


  // Reset connections
  if (state->portal_gun_connection) {
    free(state->portal_gun_connection);
  }
  if (state->box_connections) {
    list_free(state->box_connections);
  }

  // Reset platforms
  if (state->platforms) {
    list_free(state->platforms);
  }

  // Reset buttons
  if (state->buttons) {
    list_free(state->buttons);
  }

  // Initialize values
  list_set(state->scenes, scene_init(), state->curr_level);
  state->player_body = NULL;
  state->exit_body = NULL;
  state->timer_body = NULL;
  state->portal_projectile_body = NULL;
  *(state->is_jumping) = false;
  *(state->is_player_teleporting) = false;
  *(state->is_box_teleporting) = false;
  state->is_portal_restricted = false;
  state->portal1 = NULL;
  state->portal2 = NULL;
  state->portal_gun_connection = NULL;

  state->box_connections = list_init(1, free);
  state->platforms = list_init(1, free);
  state->buttons = list_init(1, free);
  state->last_time = 0;
}

/**
 * Check if an inputted portal body is colliding with
 * other bodies.
 *
 * @param state a pointer to a state
 * @param portal_body a pointer to the body of a portal
 */
bool is_colliding_with_other_bodies(state_t *state, body_t *portal_body) {
  scene_t *scene = get_curr_scene(state);

  list_t *portal_shape = body_get_shape(portal_body);

  size_t num_collided_bodies = 0;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body != portal_body) {
      list_t *shape = body_get_shape(body);
      if (find_collision(portal_shape, shape).collided) {
        num_collided_bodies += 1;
      }
      list_free(shape);
      if (num_collided_bodies > 3) { // allowed to collide with portal surface,
                                     // portal projectile, and background
        list_free(portal_shape);
        return true;
      }
    }
  }
  list_free(portal_shape);

  return false;
}

void add_portal(state_t *state, vector_t pos, vector_t direction,
                size_t portal_num);

/**
 * Check if the portal projectile is colliding with PORTAL_SURFACE.
 * If so, add a portal at the correct spot. If portal projectile
 * collides with any other surface, then remove the projectile.
 *
 * @param state a pointer to a state
 */
void check_portal_projectile_collisions(state_t *state) {
  scene_t *scene = get_curr_scene(state);

  // Check that a portal projectile is on the screen
  if (state->portal_projectile_body == NULL) {
    return;
  }

  body_t *portal_projectile_body = state->portal_projectile_body;
  list_t *portal_projectile_shape = body_get_shape(portal_projectile_body);

  // Get the portal number
  size_t portal_num = 0;
  if (get_type(portal_projectile_body) == PORTAL_PROJECTILE_1) {
    portal_num = 1;
  } else if (get_type(portal_projectile_body) == PORTAL_PROJECTILE_2) {
    portal_num = 2;
  }

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);

    if (body != portal_projectile_body) {
      list_t *shape = body_get_shape(body);
      collision_info_t collision_info =
          find_collision(shape, portal_projectile_shape);

      if (collision_info.collided) {
        if (get_type(body) == PORTAL_SURFACE) {
          vector_t projectile_centroid =
              body_get_centroid(portal_projectile_body);
          vector_t axis = collision_info.axis;

          vector_t direction_vec =
              vec_subtract(body_get_centroid(portal_projectile_body),
                           body_get_centroid(body));
          if (vec_dot(direction_vec, axis) < 0) {
            axis = vec_negate(axis);
          }

          vector_t portal_pos = vec_add(projectile_centroid,
                                        vec_multiply(-PORTAL_ADJUST_NUM, axis));

          add_portal(state, portal_pos, axis, portal_num);
          body_remove(portal_projectile_body);
          state->portal_projectile_body = NULL;
        }
        // Ignore if one of these bodies, else, remove
        else if (!(get_type(body) == PLAYER || get_type(body) == BOX ||
                   get_type(body) == PORTAL_GUN ||
                   get_type(body) == BACKGROUND)) {
          body_remove(portal_projectile_body);
          state->portal_projectile_body = NULL;
        }
      }
      list_free(shape);
    }
  }
  list_free(portal_projectile_shape);
}

/**
 * Restricts a player's speed to a preset max speed.
 *
 * @param state a pointer to a state
 */
void restrict_player_speed(state_t *state) {
  body_t *player_body = state->player_body;
  vector_t player_vel = body_get_velocity(player_body);
  double angle = vec_direction_angle(player_vel);

  if (fabs(player_vel.x) > fabs(PLAYER_MAX_SPEED * cos(angle))) {
    player_vel.x = PLAYER_MAX_SPEED * cos(angle);
  }
  if (fabs(player_vel.y) > fabs(PLAYER_MAX_SPEED * sin(angle))) {
    player_vel.y = PLAYER_MAX_SPEED * sin(angle);
  }

  body_set_velocity(player_body, player_vel);
}

/**
 * Executes a tick of every body, portal, platfor
 * in a scene over a small time interval.
 *
 * @param state a pointer to a state
 * @param dt the number of seconds elapsed since the last tick
 */
void tick_all(state_t *state, double dt) {
  scene_t *scene = get_curr_scene(state);
  portal_t *portal1 = state->portal1;
  portal_t *portal2 = state->portal2;
  body_t *player_body = state->player_body;
  bool *is_player_teleporting = state->is_player_teleporting;
  bool *is_box_teleporting = state->is_box_teleporting;
  list_t *platforms = state->platforms;
  list_t *buttons = state->buttons;
  list_t *box_connections = state->box_connections;

  // --- Portals ---
  // player
  if (state->portal1 && state->portal2) {
    portal_tick(portal1, portal2, player_body, is_player_teleporting);
    portal_tick(portal2, portal1, player_body, is_player_teleporting);
  }

  // boxes
  for (size_t i = 0; i < list_size(box_connections); i++) {
    connection_t *box_connection = list_get(box_connections, i);
    body_t *box_body = connection_get_connected_body(box_connection);
    if (state->portal1 && state->portal2) {
      portal_tick(portal1, portal2, box_body, is_box_teleporting);
      portal_tick(portal2, portal1, box_body, is_box_teleporting);
    }
  }

  check_portal_projectile_collisions(state);

  // Platforms
  for (size_t i = 0; i < list_size(platforms); i++) {
    platform_t *platform = list_get(platforms, i);
    platform_tick(platform, dt);
  }

  // Buttons
  list_t *pressing_bodies = list_init(1, NULL);
  list_add(pressing_bodies, player_body);
  for (size_t i = 0; i < list_size(box_connections); i++) {
    connection_t *box_connection = list_get(box_connections, i);
    list_add(pressing_bodies, connection_get_connected_body(box_connection));
  }
  for (size_t i = 0; i < list_size(buttons); i++) {
    button_t *button = list_get(buttons, i);
    button_tick(button, pressing_bodies, dt);
  }
  list_free(pressing_bodies);

  // Scene
  scene_tick(scene, dt);

  // Decrement timer
  state->timer -= dt;

  // Rotate portal gun
  if (state->portal_gun_connection) {
    rotate_portal_gun(state);
  }

  // Change what direction player image is facing
  if (body_get_velocity(state->player_body).x < 0) {
    body_set_image(state->player_body, state->player_left_image);
  } else if (body_get_velocity(state->player_body).x > 0) {
    body_set_image(state->player_body, state->player_right_image);
  }
}

// ----------------------- FORCES -----------------------

/**
 * A force aux for the stick force. Holds a connection between the two bodies
 * sticking.
 */
typedef struct stick_force_aux {
  connection_t *connection;
} stick_force_aux_t;

/**
 * Applies the stick force to the bodies stored in aux.
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_stick_force(void *aux) {
  stick_force_aux_t *stick_force_aux = aux;
  connection_t *connection = stick_force_aux->connection;

  body_t *body = connection_get_body(connection);
  body_t *connected_body = connection_get_connected_body(connection);
  bool is_connected = connection_get_is_connected(connection);
  vector_t displacement = connection_get_displacement(connection);

  if (is_connected) {
    vector_t new_connected_body_centroid =
        vec_add(body_get_centroid(body), displacement);
    body_set_centroid(connected_body, new_connected_body_centroid);

    // Remove all forces acting on the connected_body
    body_add_force(connected_body, vec_negate(body_get_force(connected_body)));
  }
}

/**
 * Adds a force creator to a scene that applies a stick force to bodies
 * in the connection.
 * The force creator will be called each tick to compute the stick force.
 *
 * @param scene the scene containing the bodies
 * @param connection a pointer to the connection between two bodies
 */
void create_stick_force(scene_t *scene, connection_t *connection) {
  stick_force_aux_t *stick_force_aux = calloc(1, sizeof(stick_force_aux_t));
  stick_force_aux->connection = connection;

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, connection_get_body(connection));
  list_add(bodies, connection_get_connected_body(connection));
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_stick_force,
                                 stick_force_aux, bodies, free);
}

// -----------------------  ADD BODIES  -----------------------

/**
 * Adds a body to a scene that applied a gravitational force to each other body.
 *
 * @param state a pointer to a state
 */
void add_gravity_body(state_t *state) {
  scene_t *scene = get_curr_scene(state);

  // Will be offscreen, so shape is irrelevant
  list_t *gravity_ball = make_rect_shape(1, 1);
  body_t *body = body_init_with_info(gravity_ball, M, WALL_COLOR,
                                     make_type_info(GRAVITY), free);

  // Move a distance R below the scene
  vector_t gravity_center = {CENTER.x, -R};
  body_set_centroid(body, gravity_center);
  scene_add_body(scene, body);
}

/**
 * Adds the walls to a scene to encapsulate a game level.
 *
 * @param state a pointer to a state
 * @param n the number of walls that are being added
 * @param positions a vector of length n containing the centroids of each wall
 * @param dims a vector of length n containing the dimensions of each wall
 * @param is_visible a boolean to represent if wall shapes are visible or not
 */
void add_walls(state_t *state, size_t n, vector_t positions[], vector_t dims[],
               bool is_visible) {
  scene_t *scene = get_curr_scene(state);

  for (size_t i = 0; i < n; i++) {
    body_type_t *info = make_type_info(WALL);

    list_t *shape = make_rect_shape(dims[i].x, dims[i].y);
    body_t *body = body_init_with_info(shape, INFINITY, WALL_COLOR, info, free);
    body_set_centroid(body, positions[i]);
    body_set_visibility(body, is_visible);

    scene_add_body(scene, body);
  }
}

/**
 * Adds the player body to a scene. Adds forces between the player
 * and other bodies.
 *
 * @param state a pointer to a state
 * @param player_initial_pos a vector representing the player body's starting
 * position
 */
void add_player_body(state_t *state, vector_t player_initial_pos) {
  scene_t *scene = get_curr_scene(state);

  // Initialize the player shape
  list_t *player_shape = make_rect_shape(PLAYER_DIMS.x, PLAYER_DIMS.y);

  // Initialize body
  body_t *player_body = body_init_with_info(
      player_shape, PLAYER_MASS, PLAYER_COLOR, make_type_info(PLAYER), free);
  body_set_centroid(player_body, player_initial_pos);

  state->player_left_image = sdl_load_image(PLAYER_LEFT_IMG_PATH);
  state->player_right_image = sdl_load_image(PLAYER_RIGHT_IMG_PATH);
 
  // Add force creators with other bodies
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    switch (get_type(body)) {
    case WALL:
      create_physics_collision(scene, WALL_ELASTICITY_PLAYER, player_body,
                               body);
      create_normal_force(scene, player_body, body, NULL);
      break;
    case PORTAL_SURFACE:
      create_physics_portal_collision(scene, player_body, body,
                                      state->is_player_teleporting);
      create_normal_force(scene, player_body, body,
                          state->is_player_teleporting);
      create_jump_force(scene, PLAYER_JUMP_SPEED, player_body, body,
                        state->is_jumping);
      break;
    case GRAVITY:
      create_newtonian_gravity(scene, G, player_body, body);
      break;
    case JUMPABLE:
      create_physics_collision(scene, JUMPABLE_ELASTICITY, body, player_body);
      create_normal_force(scene, player_body, body, NULL);
      create_jump_force(scene, PLAYER_JUMP_SPEED, player_body, body,
                        state->is_jumping);
      break;
    case PLATFORM:
      create_physics_collision(scene, PLATFORM_ELASTICITY, body, player_body);
      create_normal_force(scene, player_body, body, NULL);
      create_jump_force(scene, PLAYER_JUMP_SPEED, player_body, body,
                        state->is_jumping);
      break;
    case BUTTON:
      create_physics_collision(scene, BUTTON_ELASTICITY, body, player_body);
      create_normal_force(scene, player_body, body, NULL);
      create_jump_force(scene, PLAYER_JUMP_SPEED, player_body, body,
                        state->is_jumping);
    default:
      break;
    }
  }

  scene_add_body(scene, player_body);
  state->player_body = player_body;
}

/**
 * Add the gun that shoots out projectiles to make the portal to the scene.
 * Make the gun body connected to the player body.
 *
 * @param state a pointer to a state
 */
void add_portal_gun_body(state_t *state) {
  scene_t *scene = get_curr_scene(state);

  body_t *player_body = state->player_body;

  list_t *portal_gun_shape =
      make_rect_shape(PORTAL_GUN_DIMS.x, PORTAL_GUN_DIMS.y);
  body_t *portal_gun_body =
      body_init_with_info(portal_gun_shape, PORTAL_GUN_MASS, PORTAL_GUN_COLOR,
                          make_type_info(PORTAL_GUN), free);
  vector_t portal_gun_pos = body_get_centroid(player_body);
  body_set_centroid(portal_gun_body, portal_gun_pos);

  scene_add_body(scene, portal_gun_body);

  connection_t *connection = connection_init(player_body, portal_gun_body, true,
                                             PORTAL_GUN_DISPLACEMENT);
  state->portal_gun_connection = connection;

  // Make portal gun stick to player
  create_stick_force(scene, connection);
}

/**
 * Add the portal to the scene corresponding to the inputted position,
 * direction, and portal number.
 *
 * @param state a pointer to a state
 * @param pos a vector representing the position that the portal will be placed
 * @param direction a vector representing the direction which portal
 * will be facing when it's placed
 * @param portal_num a number that dictatate between the two portals:
 * 1 is blue portal, and 2 is orange portal
 */
void add_portal(state_t *state, vector_t pos, vector_t direction,
                size_t portal_num) {
  // If the portal already exists in the scene, change its position and
  // direction.
  if (portal_num == 1 && state->portal1) {
    // Apply changes
    body_t *portal_body = portal_get_body(state->portal1);
    body_set_rotation(portal_body,
                      -body_get_rotation(portal_body)); // reset angle
    body_set_rotation(portal_body, vec_direction_angle(direction));
    body_set_centroid(portal_body, pos);

    // If portal is colliding with other bodies, reverse changes
    if (is_colliding_with_other_bodies(state, portal_body)) {
      body_set_rotation(portal_body, -vec_direction_angle(direction));
      body_set_centroid(portal_body, vec_negate(pos));
    } else { // set the portal's direction
      portal_set_direction(state->portal1, direction);
    }
    return;
  }
  if (portal_num == 2 && state->portal2) {
    body_t *portal_body = portal_get_body(state->portal2);
    body_set_rotation(portal_body,
                      -body_get_rotation(portal_body)); // reset angle
    body_set_rotation(portal_body, vec_direction_angle(direction));
    body_set_centroid(portal_body, pos);

    if (is_colliding_with_other_bodies(state, portal_body)) {
      body_set_rotation(portal_body, -vec_direction_angle(direction));
      body_set_centroid(portal_body, vec_negate(pos));
    } else {
      portal_set_direction(state->portal2, direction);
    }
    return;
  }

  // Determine what color and image to use based on portal_num
  rgb_color_t portal_color = {0, 0, 0};
  const char *img_path;
  if (portal_num == 1) {
    portal_color = PORTAL1_COLOR;
    img_path = PORTAL_1_IMG_PATH;
  } else if (portal_num == 2) {
    portal_color = PORTAL2_COLOR;
    img_path = PORTAL_2_IMG_PATH;
  }

  list_t *shape = make_rect_shape(PORTAL_DIMS.x, PORTAL_DIMS.y);

  body_t *body;
  if (USE_PORTAL_IMAGES) {
    body = body_init_with_image(shape, INFINITY, portal_color,
                                make_type_info(PORTAL), free, img_path);
  } else {
    body = body_init_with_info(shape, INFINITY, portal_color,
                               make_type_info(PORTAL), free);
  }
  // Rotate portal to correct direction
  body_set_rotation(body, vec_direction_angle(direction));
  body_set_centroid(body, pos);

  if (!is_colliding_with_other_bodies(state, body)) {
    scene_t *scene = get_curr_scene(state);
    scene_add_body(scene, body);

    portal_t *portal = portal_init(body, direction);
    if (portal_num == 1) {
      state->portal1 = portal;
    } else if (portal_num == 2) {
      state->portal2 = portal;
    }
  } else {
    body_free(body);
  }
}

/**
 * Adds the exit body to a scene. If the player's body is
 * entirely in the exit body, they go to the next level.
 *
 * @param state a pointer to a state
 * @param pos a vector corresponding to the exit body's centroid
 * @param is_visible a boolean representing if the exit body is visible
 */
void add_level_exit(state_t *state, vector_t pos, bool is_visible) {
  scene_t *scene = get_curr_scene(state);

  list_t *exit_box_shape = make_rect_shape(EXIT_BOX_DIMS.x, EXIT_BOX_DIMS.y);
  body_t *exit_box_body = body_init_with_info(
      exit_box_shape, INFINITY, EXIT_BOX_COLOR, make_type_info(EXIT), free);
  body_set_centroid(exit_box_body, pos);
  body_set_visibility(exit_box_body, is_visible);

  scene_add_body(scene, exit_box_body);
  state->exit_body = exit_box_body;
}

/**
 * Adds the portal projectile to the scene.
 *
 * @param state a pointer to a state
 * @param portal_num the number of the portal (1 or 2) that a collision
 *                   of this projectile will create
 */
void add_portal_projectile(state_t *state, size_t portal_num) {
  scene_t *scene = get_curr_scene(state);
  body_t *portal_gun_body =
      connection_get_connected_body(state->portal_gun_connection);

  double portal_gun_direction =
      calculate_mouse_direction(state->mouse_pos, portal_gun_body);

  rgb_color_t portal_color = {0, 0, 0};
  body_type_t *info = NULL;
  if (portal_num == 1) {
    portal_color = PORTAL1_COLOR;
    info = make_type_info(PORTAL_PROJECTILE_1);
  } else if (portal_num == 2) {
    portal_color = PORTAL2_COLOR;
    info = make_type_info(PORTAL_PROJECTILE_2);
  }

  // Shoot portal projectile as circle
  list_t *projectile_shape =
      make_circ_shape(PORTAL_PROJECTILE_RADIUS, PORTAL_PROJECTILE_NUM_POINTS);
  body_t *projectile_body = body_init_with_info(
      projectile_shape, PORTAL_PROJECTILE_MASS, portal_color, info, free);

  vector_t projectile_vel = {
      PORTAL_PROJECTILE_SPEED * cos(portal_gun_direction),
      PORTAL_PROJECTILE_SPEED * sin(portal_gun_direction)};

  body_set_centroid(projectile_body, body_get_centroid(portal_gun_body));
  body_set_velocity(projectile_body, projectile_vel);

  scene_add_body(scene, projectile_body);
  state->portal_projectile_body = projectile_body;
}

/**
 * Adds the movable box body to a scene.
 *
 * @param state a pointer to a state
 * @param pos a vector corresponding to the box's centroid
 */
void add_box(state_t *state, vector_t pos) {
  scene_t *scene = get_curr_scene(state);
  body_t *player_body = state->player_body;

  list_t *box_shape = make_rect_shape(BOX_DIMS.x, BOX_DIMS.y);
  body_t *box_body = body_init_with_image(
      box_shape, BOX_MASS, BOX_COLOR, make_type_info(BOX), free, BOX_IMG_PATH);
  body_set_centroid(box_body, pos);

  scene_add_body(scene, box_body);

  connection_t *box_connection =
      connection_init(player_body, box_body, false, BOX_DISPLACEMENT);

  list_add(state->box_connections, box_connection);

  // Add force creators with other bodies
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    switch (get_type(body)) {
    case WALL:
      create_physics_collision(scene, WALL_ELASTICITY_BOX, box_body, body);
      create_normal_force(scene, box_body, body, NULL);
      break;
    case PORTAL_SURFACE:
      create_physics_portal_collision(scene, box_body, body,
                                      state->is_box_teleporting);
      create_normal_force(scene, box_body, body, state->is_box_teleporting);
      break;
    case GRAVITY:
      create_newtonian_gravity(scene, G, box_body, body);
      break;
    case JUMPABLE:
      create_physics_collision(scene, JUMPABLE_ELASTICITY, body, box_body);
      create_normal_force(scene, box_body, body, NULL);
      break;
    case PLATFORM:
      create_physics_collision(scene, PLATFORM_ELASTICITY, body, box_body);
      create_normal_force(scene, box_body, body, NULL);
      break;
    case BUTTON:
      create_physics_collision(scene, BUTTON_ELASTICITY, body, box_body);
      create_normal_force(scene, box_body, body, NULL);
    default:
      break;
    }
  }

  create_stick_force(scene, box_connection);
}

/**
 * Adds a button body to a scene.
 *
 * @param state a pointer to a state
 * @param pos a vector corresponding to the button's centroid
              pos: (x: center, y: bottom of base)
 * @param platforms a pointer to a list containing all the
 *                  platforms a button is connected to
 */
void add_button(state_t *state, vector_t pos, list_t *platforms) {
  scene_t *scene = get_curr_scene(state);

  button_t *button =
      button_init(pos, BUTTON_DIMS, BUTTON_COLOR, BUTTON_BASE_DIMS,
                  BUTTON_BASE_COLOR, platforms);
  list_add(state->buttons, button);

  body_t *button_body = button_get_button_body(button);
  body_t *base_body = button_get_base_body(button);

  scene_add_body(scene, button_body);
  scene_add_body(scene, base_body);
}

/**
 * Adds the timer body to a scene.
 *
 * @param state a pointer to a state
 * @param is_visible a boolean representing if the exit body is visible
 */
void add_timer(state_t *state, bool is_visible) {
  scene_t *scene = get_curr_scene(state);

  list_t *timer_shape = make_rect_shape(TIMER_DIMS.x, TIMER_DIMS.y);
  body_t *timer_body = body_init_with_info(
      timer_shape, INFINITY, TIMER_BG_COLOR, make_type_info(TIMER), free);

  body_set_centroid(timer_body, TIMER_POS);
  body_set_visibility(timer_body, is_visible);

  scene_add_body(scene, timer_body);
  state->timer_body = timer_body;
}

/**
 * Adds the surfaces that a player body or box can stand on.
 *
 * @param state a pointer to a state
 * @param n the number of standing surfaces being added
 * @param positions a vector list containing the centroids of the surfaces
 * @param dims a vector list containing the dimensions of the surfaces
 * @param is_visible a boolean representing if the surface shapes are visible or
 * not
 */
void add_standing_surfaces(state_t *state, size_t n, vector_t positions[],
                           vector_t dims[], bool is_visible) {
  scene_t *scene = get_curr_scene(state);

  for (size_t i = 0; i < n; i++) {
    body_type_t *info = make_type_info(JUMPABLE);

    list_t *shape = make_rect_shape(dims[i].x, dims[i].y);
    body_t *body = body_init_with_info(shape, INFINITY, STANDING_SURFACE_COLOR,
                                       info, free);
    body_set_centroid(body, positions[i]);
    body_set_visibility(body, is_visible);

    scene_add_body(scene, body);
  }
}

/**
 * Adds the surfaces that can have a portal to the scene.
 *
 * @param state a pointer to a state
 * @param n the number of surfaces in the scene
 * @param positions a vector list with the centroids of the surfaces being added
 * @param dims a vector list with the dimensions of the surfaces being added
 * @param is_visible a boolean representing whether the surface shapes are
 * visible or not
 */
void add_portal_surfaces(state_t *state, size_t n, vector_t positions[],
                         vector_t dims[], bool is_visible) {
  scene_t *scene = get_curr_scene(state);

  for (size_t i = 0; i < n; i++) {
    body_type_t *info = make_type_info(PORTAL_SURFACE);

    list_t *shape = make_rect_shape(dims[i].x, dims[i].y);
    body_t *body =
        body_init_with_info(shape, INFINITY, PORTAL_SURFACE_COLOR, info, free);
    body_set_centroid(body, positions[i]);
    body_set_visibility(body, is_visible);

    scene_add_body(scene, body);
  }
}

/**
 * Adds the background of a scene.
 *
 * @param state a pointer to a state
 * @param image_path a pointer to an image path containing the background
 */
void add_background(state_t *state, char *image_path) {
  scene_t *scene = get_curr_scene(state);

  list_t *shape = make_rect_shape(WINDOW.x, WINDOW.y);
  body_t *body =
      body_init_with_image(shape, INFINITY, (rgb_color_t){0.5, 0.5, 0.5},
                           make_type_info(BACKGROUND), free, image_path);
  body_set_centroid(body, CENTER);

  scene_add_body(scene, body);
}

// ----------------- START SCREEN  -------------------

/**
 * Initializes the scene containing the start screen.
 *
 * @param state a pointer to a state
 */
void start_screen_init(state_t *state) {
  reset_level(state);
  scene_t *scene = get_curr_scene(state);

  list_t *shape = make_rect_shape(WINDOW.x, WINDOW.y);
  body_t *body = body_init_with_image(shape, INFINITY, (rgb_color_t){0, 0, 0},
                                      NULL, NULL, START_SCREEN_IMG_PATH);
  body_set_centroid(body, CENTER);

  scene_add_body(scene, body);
}

/**
 * Run the start screen.
 *
 * @param state a pointer to a state
 */
void start_screen_main(state_t *state, double dt) {
  scene_t *scene = get_curr_scene(state);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// -------------------  GAME WON SCREEN  -------------------

/**
 * Initializes the "Game Won" screen.
 *
 * @param state a pointer to a state
 */
void game_won_screen_init(state_t *state) {
  reset_level(state);
  scene_t *scene = get_curr_scene(state);

  list_t *shape = make_rect_shape(WINDOW.x, WINDOW.y);
  body_t *body = body_init_with_image(shape, INFINITY, (rgb_color_t){0, 0, 0},
                                      NULL, NULL, GAME_WON_IMG_PATH);
  body_set_centroid(body, CENTER);

  scene_add_body(scene, body);
}

/**
 * Run the "Game Won" screen.
 *
 * @param state a pointer to a state
 */
void game_won_screen_main(state_t *state, double dt) {
  scene_t *scene = get_curr_scene(state);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// -------------------  LEVEL SCREEN  -------------------
/**
 * Initializes the level select screen.
 *
 * @param state a pointer to a state
 */
void level_screen_init(state_t *state) {
  reset_level(state);
  scene_t *scene = get_curr_scene(state);

  list_t *shape = make_rect_shape(WINDOW.x, WINDOW.y);
  body_t *body = body_init_with_image(shape, INFINITY, (rgb_color_t){0, 0, 0},
                                      NULL, NULL, LEVEL_SCREEN_IMG_PATH);
  body_set_centroid(body, CENTER);

  scene_add_body(scene, body);
}

/**
 * Run the level select screen.
 *
 * @param state a pointer to a state
 */
void level_screen_main(state_t *state, double dt) {
  scene_t *scene = get_curr_scene(state);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// -------------------  RULES SCREEN  -------------------

/**
 * Initializes the rules screen scene.
 *
 * @param state a pointer to a state
 */
void rules_screen_init(state_t *state) {
  reset_level(state);
  scene_t *scene = get_curr_scene(state);

  list_t *shape = make_rect_shape(WINDOW.x, WINDOW.y);
  body_t *body = body_init_with_image(shape, INFINITY, (rgb_color_t){0, 0, 0},
                                      NULL, NULL, RULES_SCREEN_IMG_PATH);
  body_set_centroid(body, CENTER);

  scene_add_body(scene, body);
}

/**
 * Run the rules screen.
 *
 * @param state a pointer to a state
 */
void rules_screen_main(state_t *state, double dt) {
  scene_t *scene = get_curr_scene(state);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// -----------------------  LEVEL 0  -----------------------

/**
 * Adds all of the physical platforms to level 0
 *
 * @param state a pointer to a state
 */
void add_level_0_platforms(state_t *state) {
  scene_t *scene = get_curr_scene(state);

  size_t num_platforms = 2;

  vector_t platform_positions[num_platforms];
  platform_positions[0] = (vector_t){448, 416};
  platform_positions[1] = (vector_t){576, 392};

  float platform_motion_times[num_platforms];
  platform_motion_times[0] = 2;
  platform_motion_times[1] = 2;

  float platform_rotations[num_platforms];
  platform_rotations[0] = deg_to_rad(-25);
  platform_rotations[1] = 0;

  vector_t platform_translations[num_platforms];
  platform_translations[0] = (vector_t){38, 23};
  platform_translations[1] = (vector_t){0, 48};

  vector_t platform_points_of_rotations[num_platforms];
  platform_points_of_rotations[0] = (vector_t){447, 340};
  platform_points_of_rotations[1] = (vector_t){598, 340};

  for (size_t i = 0; i < num_platforms; i++) {
    vector_t platform_pos = platform_positions[i];
    float platform_motion_time = platform_motion_times[i];
    float platform_rotation = platform_rotations[i];
    vector_t platform_translation = platform_translations[i];
    vector_t platform_points_of_rotation = platform_points_of_rotations[i];

    body_t *platform_body = body_init_with_info(
        make_rect_shape(PLATFORM_DIMS.x, PLATFORM_DIMS.y), INFINITY,
        PLATFORM_COLOR, make_type_info(PLATFORM), free);
    body_set_centroid(platform_body, platform_pos);
    body_set_rotation(platform_body, platform_rotation);
    scene_add_body(scene, platform_body);

    platform_t *platform =
        platform_init(platform_body, platform_motion_time, -platform_rotation,
                      platform_translation, platform_points_of_rotation);
    list_add(state->platforms, platform);
  }
}

/**
 * Initializes the scene and level layout for level 0.
 *
 * @param state a pointer to a state
 */
void level_0_init(state_t *state) {
  state->timer = 60;
  reset_level(state);
  state->is_portal_restricted = false;

  vector_t player_initial_pos = {100, PLAYER_DIMS.y / 2 + WALL_THICKNESS};

  vector_t portal1_pos = {WINDOW.x - PORTAL_DIMS.x / 2 - WALL_THICKNESS,
                          PORTAL_DIMS.y / 2 + WALL_THICKNESS};
  vector_t portal2_pos = {PORTAL_DIMS.x / 2 + WALL_THICKNESS,
                          PORTAL_DIMS.y / 2 + 7 * 64};
  vector_t portal1_dir = {-1, 0};
  vector_t portal2_dir = {1, 0};

  vector_t box_pos = {224, 7 * 64 + BOX_DIMS.y / 2};

  vector_t button_pos = {CENTER.x, WALL_THICKNESS};

  vector_t exit_pos = {WINDOW.x - 2 * 64 - EXIT_BOX_DIMS.x / 2,
                       WINDOW.y - 3 * 64 - EXIT_BOX_DIMS.y / 2};

  size_t num_walls = 7;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){32.0, 624.0};
  wall_positions[1] = (vector_t){24.0, 496.0};
  wall_positions[2] = (vector_t){32.0, 224.0};
  wall_positions[3] = (vector_t){992.0, 32.0};
  wall_positions[4] = (vector_t){1000.0, 112.0};
  wall_positions[5] = (vector_t){992.0, 432.0};
  wall_positions[6] = (vector_t){512.0, 672.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){64.0, 160.0};
  wall_dims[1] = (vector_t){48.0, 96.0};
  wall_dims[2] = (vector_t){64.0, 448.0};
  wall_dims[3] = (vector_t){64.0, 64.0};
  wall_dims[4] = (vector_t){48.0, 96.0};
  wall_dims[5] = (vector_t){64.0, 544.0};
  wall_dims[6] = (vector_t){896.0, 64.0};

  size_t num_standing_surfaces = 6;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){224.0, 416.0};
  standing_surface_positions[1] = (vector_t){352.0, 352.0};
  standing_surface_positions[2] = (vector_t){512.0, 288.0};
  standing_surface_positions[3] = (vector_t){672.0, 352.0};
  standing_surface_positions[4] = (vector_t){800.0, 416.0};
  standing_surface_positions[5] = (vector_t){512.0, 32.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){320.0, 64.0};
  standing_surface_dims[1] = (vector_t){64.0, 64.0};
  standing_surface_dims[2] = (vector_t){384.0, 64.0};
  standing_surface_dims[3] = (vector_t){64.0, 64.0};
  standing_surface_dims[4] = (vector_t){320.0, 64.0};
  standing_surface_dims[5] = (vector_t){896.0, 64.0};

  size_t num_portal_surfaces = 2;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){56.0, 496.0};
  portal_surface_positions[1] = (vector_t){968.0, 112.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){16.0, 96.0};
  portal_surface_dims[1] = (vector_t){16.0, 96.0};

  char *bg_filepath = "assets/images/level_0.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_portal(state, portal1_pos, portal1_dir, 1);
  add_portal(state, portal2_pos, portal2_dir, 2);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);
  add_level_0_platforms(state);

  list_t *button_platforms = list_init(2, NULL);
  list_add(button_platforms, list_get(state->platforms, 0));
  list_add(button_platforms, list_get(state->platforms, 1));
  add_button(state, button_pos, button_platforms);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_right_image);
  add_box(state, box_pos);

  add_timer(state, false);
}

/**
 * Run level 0.
 *
 * @param state a pointer to a state
 */
void level_0_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);

  sdl_render_scene(scene);
}

// -----------------------  LEVEL 1  -----------------------
/**
 * Initializes the scene and level layout for level 1
 *
 * @param state a pointer to a state
 */
void level_1_init(state_t *state) {
  reset_level(state);
  state->timer = 60;
  state->is_portal_restricted = false;

  vector_t player_initial_pos = {2 * 64, 4 * 64 + PLAYER_DIMS.y / 2};

  vector_t exit_pos = {384, 544};

  size_t num_walls = 9;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){160.0, 672.0};
  wall_positions[1] = (vector_t){24.0, 448.0};
  wall_positions[2] = (vector_t){32.0, 128.0};
  wall_positions[3] = (vector_t){992.0, 32.0};
  wall_positions[4] = (vector_t){1000.0, 160.0};
  wall_positions[5] = (vector_t){992.0, 288.0};
  wall_positions[6] = (vector_t){1000.0, 480.0};
  wall_positions[7] = (vector_t){992.0, 672.0};
  wall_positions[8] = (vector_t){720.0, 680.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){320.0, 64.0};
  wall_dims[1] = (vector_t){48.0, 384.0};
  wall_dims[2] = (vector_t){64.0, 256.0};
  wall_dims[3] = (vector_t){64.0, 64.0};
  wall_dims[4] = (vector_t){48.0, 192.0};
  wall_dims[5] = (vector_t){64.0, 64.0};
  wall_dims[6] = (vector_t){48.0, 320.0};
  wall_dims[7] = (vector_t){64.0, 64.0};
  wall_dims[8] = (vector_t){480.0, 48.0};

  size_t num_standing_surfaces = 5;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){288.0, 544.0};
  standing_surface_positions[1] = (vector_t){480.0, 480.0};
  standing_surface_positions[2] = (vector_t){192.0, 160.0};
  standing_surface_positions[3] = (vector_t){768.0, 288.0};
  standing_surface_positions[4] = (vector_t){512.0, 32.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){64.0, 192.0};
  standing_surface_dims[1] = (vector_t){320.0, 64.0};
  standing_surface_dims[2] = (vector_t){256.0, 192.0};
  standing_surface_dims[3] = (vector_t){384.0, 64.0};
  standing_surface_dims[4] = (vector_t){896.0, 64.0};

  size_t num_portal_surfaces = 4;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){56.0, 448.0};
  portal_surface_positions[1] = (vector_t){720.0, 648.0};
  portal_surface_positions[2] = (vector_t){968.0, 480.0};
  portal_surface_positions[3] = (vector_t){968.0, 160.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){16.0, 384.0};
  portal_surface_dims[1] = (vector_t){480.0, 16.0};
  portal_surface_dims[2] = (vector_t){16.0, 320.0};
  portal_surface_dims[3] = (vector_t){16.0, 192.0};

  char *bg_filepath = "assets/images/level_1.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_right_image);

  add_timer(state, false);

  add_portal_gun_body(state);
}

/**
 * Runs level 1
 *
 * @param state a pointer to a state
 */
void level_1_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);
  sdl_render_scene(scene);
}

// -----------------------  LEVEL 2  -----------------------

/**
 * Initializes the scene and level layout for level 2
 *
 * @param state a pointer to a state
 */
void level_2_init(state_t *state) {
  reset_level(state);
  state->timer = 60;
  state->is_portal_restricted = false;

  vector_t player_initial_pos = {2 * 64, 4 * 64 + PLAYER_DIMS.y / 2};

  vector_t exit_pos = {768, 96};

  size_t num_walls = 8;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){32.0, 688.0};
  wall_positions[1] = (vector_t){24.0, 592.0};
  wall_positions[2] = (vector_t){32.0, 480.0};
  wall_positions[3] = (vector_t){24.0, 288.0};
  wall_positions[4] = (vector_t){32.0, 64.0};
  wall_positions[5] = (vector_t){992.0, 688.0};
  wall_positions[6] = (vector_t){1000.0, 368.0};
  wall_positions[7] = (vector_t){992.0, 32.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){64.0, 32.0};
  wall_dims[1] = (vector_t){48.0, 160.0};
  wall_dims[2] = (vector_t){64.0, 64.0};
  wall_dims[3] = (vector_t){48.0, 320.0};
  wall_dims[4] = (vector_t){64.0, 128.0};
  wall_dims[5] = (vector_t){64.0, 32.0};
  wall_dims[6] = (vector_t){48.0, 608.0};
  wall_dims[7] = (vector_t){64.0, 64.0};

  size_t num_standing_surfaces = 9;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){128.0, 480.0};
  standing_surface_positions[1] = (vector_t){192.0, 64.0};
  standing_surface_positions[2] = (vector_t){608.0, 672.0};
  standing_surface_positions[3] = (vector_t){608.0, 512.0};
  standing_surface_positions[4] = (vector_t){736.0, 480.0};
  standing_surface_positions[5] = (vector_t){808.0, 384.0};
  standing_surface_positions[6] = (vector_t){736.0, 288.0};
  standing_surface_positions[7] = (vector_t){672.0, 128.0};
  standing_surface_positions[8] = (vector_t){832.0, 32.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){128.0, 64.0};
  standing_surface_dims[1] = (vector_t){256.0, 128.0};
  standing_surface_dims[2] = (vector_t){64.0, 64.0};
  standing_surface_dims[3] = (vector_t){64.0, 128.0};
  standing_surface_dims[4] = (vector_t){192.0, 64.0};
  standing_surface_dims[5] = (vector_t){48.0, 128.0};
  standing_surface_dims[6] = (vector_t){192.0, 64.0};
  standing_surface_dims[7] = (vector_t){64.0, 256.0};
  standing_surface_dims[8] = (vector_t){256.0, 64.0};

  size_t num_portal_surfaces = 4;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){56.0, 592.0};
  portal_surface_positions[1] = (vector_t){56.0, 288.0};
  portal_surface_positions[2] = (vector_t){968.0, 368.0};
  portal_surface_positions[3] = (vector_t){776.0, 384.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){16.0, 160.0};
  portal_surface_dims[1] = (vector_t){16.0, 320.0};
  portal_surface_dims[2] = (vector_t){16.0, 608.0};
  portal_surface_dims[3] = (vector_t){16.0, 128.0};

  char *bg_filepath = "assets/images/level_2.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_right_image);

  add_timer(state, false);

  add_portal_gun_body(state);
}

/**
 * Run level 2.
 *
 * @param state a pointer to a state
 */
void level_2_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);
  sdl_render_scene(scene);
}

// -----------------------  LEVEL 3  -----------------------

void add_level_3_platforms(state_t *state) {
  scene_t *scene = get_curr_scene(state);

  size_t num_platforms = 2;

  vector_t platform_positions[num_platforms];
  platform_positions[0] = (vector_t){648, 112};
  platform_positions[1] = (vector_t){888, 112};

  float platform_motion_times[num_platforms];
  platform_motion_times[0] = 2;
  platform_motion_times[1] = 2;

  float platform_rotations[num_platforms];
  platform_rotations[0] = deg_to_rad(-90);
  platform_rotations[1] = deg_to_rad(90);

  vector_t platform_translations[num_platforms];
  platform_translations[0] = (vector_t){0, 0};
  platform_translations[1] = (vector_t){0, 0};

  vector_t platform_points_of_rotations[num_platforms];
  platform_points_of_rotations[0] = (vector_t){640, 176};
  platform_points_of_rotations[1] = (vector_t){896, 176};

  for (size_t i = 0; i < num_platforms; i++) {
    vector_t platform_pos = platform_positions[i];
    float platform_motion_time = platform_motion_times[i];
    float platform_rotation = platform_rotations[i];
    vector_t platform_translation = platform_translations[i];
    vector_t platform_points_of_rotation = platform_points_of_rotations[i];

    body_t *platform_body = body_init_with_info(
        make_rect_shape(PLATFORM_DIMS.x, PLATFORM_DIMS.y), INFINITY,
        PLATFORM_COLOR, make_type_info(PLATFORM), free);
    body_set_centroid(platform_body, platform_pos);
    body_set_rotation(platform_body, platform_rotation);
    scene_add_body(scene, platform_body);

    platform_t *platform =
        platform_init(platform_body, platform_motion_time, -platform_rotation,
                      platform_translation, platform_points_of_rotation);
    list_add(state->platforms, platform);
  }
}

/**
 * Initializes the scene and level layout for level 3
 *
 * @param state a pointer to a state
 */
void level_3_init(state_t *state) {
  reset_level(state);
  state->timer = 60;
  state->is_portal_restricted = false;

  vector_t player_initial_pos = {2 * 64, 4 * 64 + PLAYER_DIMS.y / 2};

  vector_t exit_pos = {928, 224};

  size_t num_walls = 9;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){32.0, 640.0};
  wall_positions[1] = (vector_t){24.0, 496.0};
  wall_positions[2] = (vector_t){32.0, 384.0};
  wall_positions[3] = (vector_t){24.0, 272.0};
  wall_positions[4] = (vector_t){32.0, 96.0};
  wall_positions[5] = (vector_t){992.0, 64.0};
  wall_positions[6] = (vector_t){992.0, 688.0};
  wall_positions[7] = (vector_t){1000.0, 592.0};
  wall_positions[8] = (vector_t){992.0, 480.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){64.0, 128.0};
  wall_dims[1] = (vector_t){48.0, 160.0};
  wall_dims[2] = (vector_t){64.0, 64.0};
  wall_dims[3] = (vector_t){48.0, 160.0};
  wall_dims[4] = (vector_t){64.0, 192.0};
  wall_dims[5] = (vector_t){64.0, 128.0};
  wall_dims[6] = (vector_t){64.0, 32.0};
  wall_dims[7] = (vector_t){48.0, 160.0};
  wall_dims[8] = (vector_t){64.0, 64.0};

  size_t num_standing_surfaces = 4;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){128.0, 384.0};
  standing_surface_positions[1] = (vector_t){352.0, 96.0};
  standing_surface_positions[2] = (vector_t){864.0, 480.0};
  standing_surface_positions[3] = (vector_t){960.0, 160.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){128.0, 64.0};
  standing_surface_dims[1] = (vector_t){576.0, 192.0};
  standing_surface_dims[2] = (vector_t){192.0, 64.0};
  standing_surface_dims[3] = (vector_t){128.0, 64.0};

  size_t num_portal_surfaces = 3;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){56.0, 496.0};
  portal_surface_positions[1] = (vector_t){56.0, 272.0};
  portal_surface_positions[2] = (vector_t){968.0, 592.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){16.0, 160.0};
  portal_surface_dims[1] = (vector_t){16.0, 160.0};
  portal_surface_dims[2] = (vector_t){16.0, 160.0};

  vector_t box_1_pos = {144, 432};
  vector_t box_2_pos = {848, 528};

  vector_t button_1_pos = {256, 192};
  vector_t button_2_pos = {512, 192};

  char *bg_filepath = "assets/images/level_3.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);
  add_level_3_platforms(state);

  list_t *button_1_platforms = list_init(1, NULL);
  list_add(button_1_platforms, list_get(state->platforms, 0));
  list_t *button_2_platforms = list_init(1, NULL);
  list_add(button_2_platforms, list_get(state->platforms, 1));
  add_button(state, button_1_pos, button_1_platforms);
  add_button(state, button_2_pos, button_2_platforms);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_right_image);

  add_box(state, box_1_pos);
  add_box(state, box_2_pos);

  add_timer(state, false);

  add_portal_gun_body(state);
}

/**
 * Run level 3
 *
 * @param state a pointer to a state
 */
void level_3_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);
  sdl_render_scene(scene);
}

// -----------------------  LEVEL 4  -----------------------
/**
 * Initializes the scene and level layout of level 4
 *
 * @param state a pointer to a state
 */
void level_4_init(state_t *state) {
  reset_level(state);
  state->timer = 60;
  state->is_portal_restricted = true;

  vector_t player_initial_pos = {832, 3 * 64 + PLAYER_DIMS.y / 2};

  vector_t exit_pos = {864, 416};

  size_t num_walls = 3;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){32.0, 352.0};
  wall_positions[1] = (vector_t){992.0, 352.0};
  wall_positions[2] = (vector_t){832.0, 672.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){64.0, 704.0};
  wall_dims[1] = (vector_t){64.0, 704.0};
  wall_dims[2] = (vector_t){256.0, 64.0};

  size_t num_standing_surfaces = 6;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){312.0, 32.0};
  standing_surface_positions[1] = (vector_t){808.0, 32.0};
  standing_surface_positions[2] = (vector_t){832.0, 128.0};
  standing_surface_positions[3] = (vector_t){832.0, 360.0};
  standing_surface_positions[4] = (vector_t){416.0, 352.0};
  standing_surface_positions[5] = (vector_t){608.0, 16.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){496.0, 64.0};
  standing_surface_dims[1] = (vector_t){304.0, 64.0};
  standing_surface_dims[2] = (vector_t){256.0, 128.0};
  standing_surface_dims[3] = (vector_t){256.0, 48.0};
  standing_surface_dims[4] = (vector_t){192.0, 64.0};
  standing_surface_dims[5] = (vector_t){96.0, 32.0};

  size_t num_portal_surfaces = 2;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){608.0, 48.0};
  portal_surface_positions[1] = (vector_t){832.0, 328.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){128, 32.0};
  portal_surface_dims[1] = (vector_t){256.0, 16.0};

  char *bg_filepath = "assets/images/level_4.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);

  // Slanted shape on left of screen is a portal surface
  list_t *slanted_shape = list_init(1, free);
  vector_t *v;
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){64, 352};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){64, 320};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){128, 256};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){160, 256};
  list_add(slanted_shape, v);
  body_t *slanted_body =
      body_init_with_info(slanted_shape, INFINITY, PORTAL_SURFACE_COLOR,
                          make_type_info(PORTAL_SURFACE), free);
  vector_t slanted_body_centroid = {104, 330};
  body_set_centroid(slanted_body, slanted_body_centroid);
  body_set_visibility(slanted_body, false);
  scene_add_body(get_curr_scene(state), slanted_body);

  vector_t portal_2_pos = {608, 70};
  vector_t portal_2_dir = {0, 1};
  add_portal(state, portal_2_pos, portal_2_dir, 2);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_left_image);

  add_timer(state, false);

  add_portal_gun_body(state);
}

/**
 * Run level 4.
 *
 * @param state a pointer to a state
 */
void level_4_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);
  sdl_render_scene(scene);
}

// -----------------------  LEVEL 5  -----------------------

/**
 * Initializes the scene and level layout for level 5.
 *
 * @param state a pointer to a state
 */
void level_5_init(state_t *state) {
  reset_level(state);
  state->timer = 60;
  state->is_portal_restricted = false;

  vector_t player_initial_pos = {5 * 64, 1 * 64 + PLAYER_DIMS.y / 2};

  vector_t exit_pos = {2 * 64 + EXIT_BOX_DIMS.x / 2,
                       7 * 64 + EXIT_BOX_DIMS.y / 2};

  size_t num_walls = 5;
  vector_t wall_positions[num_walls];
  wall_positions[0] = (vector_t){512.0, 672.0};
  wall_positions[1] = (vector_t){32.0, 320.0};
  wall_positions[2] = (vector_t){784.0, 96.0};
  wall_positions[3] = (vector_t){992.0, 320.0};
  wall_positions[4] = (vector_t){160.0, 192.0};
  vector_t wall_dims[num_walls];
  wall_dims[0] = (vector_t){1024.0, 64.0};
  wall_dims[1] = (vector_t){64.0, 640.0};
  wall_dims[2] = (vector_t){32.0, 64.0};
  wall_dims[3] = (vector_t){64.0, 640.0};
  wall_dims[4] = (vector_t){192.0, 384.0};

  size_t num_standing_surfaces = 5;
  vector_t standing_surface_positions[num_standing_surfaces];
  standing_surface_positions[0] = (vector_t){256.0, 416.0};
  standing_surface_positions[1] = (vector_t){544.0, 368.0};
  standing_surface_positions[2] = (vector_t){368.0, 32.0};
  standing_surface_positions[3] = (vector_t){544.0, 16.0};
  standing_surface_positions[4] = (vector_t){784.0, 32.0};
  vector_t standing_surface_dims[num_standing_surfaces];
  standing_surface_dims[0] = (vector_t){384.0, 64.0};
  standing_surface_dims[1] = (vector_t){128.0, 32.0};
  standing_surface_dims[2] = (vector_t){224.0, 64.0};
  standing_surface_dims[3] = (vector_t){128.0, 32.0};
  standing_surface_dims[4] = (vector_t){352.0, 64.0};

  size_t num_portal_surfaces = 2;
  vector_t portal_surface_positions[num_portal_surfaces];
  portal_surface_positions[0] = (vector_t){544.0, 336.0};
  portal_surface_positions[1] = (vector_t){544.0, 48.0};
  vector_t portal_surface_dims[num_portal_surfaces];
  portal_surface_dims[0] = (vector_t){128.0, 32.0};
  portal_surface_dims[1] = (vector_t){128.0, 32.0};

  char *bg_filepath = "assets/images/level_5.png";
  add_background(state, bg_filepath);

  add_gravity_body(state);
  add_walls(state, num_walls, wall_positions, wall_dims, false);
  add_level_exit(state, exit_pos, false);
  add_standing_surfaces(state, num_standing_surfaces,
                        standing_surface_positions, standing_surface_dims,
                        false);
  add_portal_surfaces(state, num_portal_surfaces, portal_surface_positions,
                      portal_surface_dims, false);

  // Slanted shape on left of screen is a portal surface
  list_t *slanted_shape = list_init(1, free);
  vector_t *v;
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){768, 128};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){800, 128};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){960, 288};
  list_add(slanted_shape, v);
  v = calloc(1, sizeof(vector_t));
  *v = (vector_t){960, 320};
  list_add(slanted_shape, v);
  body_t *slanted_body =
      body_init_with_info(slanted_shape, INFINITY, PORTAL_SURFACE_COLOR,
                          make_type_info(PORTAL_SURFACE), free);
  vector_t slanted_body_centroid = {871.8, 216.2};
  body_set_centroid(slanted_body, slanted_body_centroid);
  body_set_visibility(slanted_body, false);
  scene_add_body(get_curr_scene(state), slanted_body);

  add_player_body(state, player_initial_pos);
  body_set_image(state->player_body, state->player_right_image);

  add_timer(state, false);

  add_portal_gun_body(state);
}

/**
 * Run level 5.
 *
 * @param state a pointer to a state
 */
void level_5_main(state_t *state, double dt) {
  display_timer(state);
  tick_all(state, dt);

  scene_t *scene = get_curr_scene(state);
  sdl_render_scene(scene);
}

/**
 * Keyboard handler.
 *
 * @param state pointer to the current state of the program
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
void on_key(state_t *state, char key, key_event_type_t type, double held_time) {
  body_t *player_body = state->player_body;
  bool *is_jumping = state->is_jumping;
  list_t *box_connections = state->box_connections;

  if (type == KEY_RELEASED) {
    *is_jumping = false;
  } else if (key == RIGHT_ARROW || key == D) {
    vector_t force = {PLAYER_MOVE_FORCE_MAG, 0};
    body_add_force(player_body, force);
  } else if (key == LEFT_ARROW || key == A) {
    vector_t force = {-PLAYER_MOVE_FORCE_MAG, 0};
    body_add_force(player_body, force);
  } else if (key == UP_ARROW || key == W) {
    vector_t player_velocity = body_get_velocity(player_body);
    if (fabs(player_velocity.y) <= PLAYER_JUMP_THRESHOLD) {
      *is_jumping = true;
    }
  } else if (key == Q) {
    sdl_play_sound(PORTAL_GUN_SOUND_PATH);
    add_portal_projectile(state, 1);
  } else if (key == E && !state->is_portal_restricted) {
    sdl_play_sound(PORTAL_GUN_SOUND_PATH);
    add_portal_projectile(state, 2);
  } else if (key == F) {
    list_t *player_shape = body_get_shape(player_body);
    for (size_t i = 0; i < list_size(box_connections); i++) {
      connection_t *box_connection = list_get(box_connections, i);
      body_t *box_body = connection_get_connected_body(box_connection);
      bool is_connected = connection_get_is_connected(box_connection);

      list_t *box_shape = body_get_shape(box_body);

      collision_info_t collision_info = find_collision(player_shape, box_shape);
      if (collision_info.collided || is_connected) {
        connection_toggle(box_connection);
        break;
      }
      list_free(box_shape);
    }
    list_free(player_shape);
  } else if (key == RET) {
    if (state->curr_level == START_SCREEN_IDX) {
      state->curr_level = 0;
      init_new_level(state);
    }
    if (state->curr_level == GAME_WON_SCREEN_IDX) {
      state->curr_level = START_SCREEN_IDX;
      init_new_level(state);
    }
  } else if (key == ESC) {
    if (state->curr_level == START_SCREEN_IDX) {
      state->curr_level = LEVEL_SCREEN_IDX;
      init_new_level(state);
    } else if (state->curr_level != GAME_WON_SCREEN_IDX) {
      state->curr_level = START_SCREEN_IDX;
      init_new_level(state);
    }
  } else if (key == RULES) {
    if (state->curr_level == START_SCREEN_IDX) {
      state->curr_level = RULES_SCREEN_IDX;
      init_new_level(state);
    }
  } else if (state->curr_level == LEVEL_SCREEN_IDX) {
    if (key == ONE) {
      state->curr_level = 0;
    } else if (key == TWO) {
      state->curr_level = 1;
    } else if (key == THREE) {
      state->curr_level = 2;
    } else if (key == FOUR) {
      state->curr_level = 3;
    } else if (key == FIVE) {
      state->curr_level = 4;
    } else if (key == SIX) {
      state->curr_level = 5;
    } else if (key == SEVEN) {
      state->curr_level = 6;
    } else if (key == EIGHT) {
      state->curr_level = 7;
    }
    init_new_level(state);
  }

  // Reset x component of velocity
  vector_t new_velocity = (vector_t){0, body_get_velocity(player_body).y};
  body_set_velocity(player_body, new_velocity);
}

/**
 * Initialize eitehr the game level, start screen, level screen, or game won
 * screen according to the current level in state.
 *
 * @param state a pointer to a state
 */
void init_new_level(state_t *state) {
  switch (state->curr_level) {
  case 0:
    level_0_init(state);
    break;
  case 1:
    level_1_init(state);
    break;
  case 2:
    level_2_init(state);
    break;
  case 3:
    level_3_init(state);
    break;
  case 4:
    level_4_init(state);
    break;
  case 5:
    level_5_init(state);
    break;
  case START_SCREEN_IDX:
    start_screen_init(state);
    break;
  case GAME_WON_SCREEN_IDX:
    game_won_screen_init(state);
    break;
  case LEVEL_SCREEN_IDX:
    level_screen_init(state);
    break;
  case RULES_SCREEN_IDX:
    rules_screen_init(state);
    break;
  }
}

/**
 * Runs the correct scene based on the current level in the state.
 *
 * @param state a pointer to a state
 * @param dt seconds passed since the last tick
 */
void run_curr_level(state_t *state, double dt) {
  switch (state->curr_level) {
  case 0:
    level_0_main(state, dt);
    break;
  case 1:
    level_1_main(state, dt);
    break;
  case 2:
    level_2_main(state, dt);
    break;
  case 3:
    level_3_main(state, dt);
    break;
  case 4:
    level_4_main(state, dt);
    break;
  case 5:
    level_5_main(state, dt);
    break;
  case START_SCREEN_IDX:
    start_screen_main(state, dt);
    break;
  case GAME_WON_SCREEN_IDX:
    game_won_screen_main(state, dt);
    break;
  case LEVEL_SCREEN_IDX:
    level_screen_main(state, dt);
    break;
  case RULES_SCREEN_IDX:
    rules_screen_main(state, dt);
    break;
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
  if (TTF_Init() == -1) {
    printf("TTF_Init: %s\n", TTF_GetError());
    exit(2);
  }

  state_t *state = calloc(1, sizeof(state_t));
  state->scenes = list_init(NUM_LEVELS, (free_func_t)scene_free);
  state->curr_level = START_SCREEN_IDX;
  state->is_jumping = calloc(1, sizeof(bool));
  state->is_player_teleporting = calloc(1, sizeof(bool));
  state->is_box_teleporting = calloc(1, sizeof(bool));

  // Initialize empty scenes
  for (size_t i = 0; i < NUM_LEVELS + 4; i++) {
    list_add(state->scenes, scene_init());
  }

  init_new_level(state);
  sdl_on_key(on_key);
  sdl_start_background_music(BACKGROUND_MUSIC_FILE_PATH);
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

  run_curr_level(state, dt);
  restrict_player_speed(state);
  check_end_level(state);

  state->mouse_pos = sdl_get_mouse_pos();
}

/**
 * Free memory allocated to the state and contents within.
 *
 * @param state a pointer to the state of the program
 */
void emscripten_free(state_t *state) {
  list_free(state->scenes);
  free(state->is_jumping);
  free(state->is_player_teleporting);
  free(state->is_box_teleporting);
  if (state->portal1) {
    portal_free(state->portal1);
  }
  if (state->portal2) {
    portal_free(state->portal2);
  }
  if (state->portal_gun_connection) {
    free(state->portal_gun_connection);
  }
  if (state->box_connections) {
    list_free(state->box_connections);
  }
  if (state->platforms) {
    list_free(state->platforms);
  }
  if (state->buttons) {
    list_free(state->buttons);
  }
  free(state);
}
