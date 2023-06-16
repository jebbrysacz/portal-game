#include "../include/body.h"
#include <stdlib.h>

typedef enum {
  PLAYER,
  WALL,
  JUMPABLE,
  GRAVITY,
  PORTAL,
  PORTAL_GUN,
  PORTAL_PROJECTILE,
  PORTAL_PROJECTILE_1,
  PORTAL_PROJECTILE_2,
  BOX,
  BUTTON,
  PLATFORM,
  EXIT,
  PORTAL_SURFACE,
  TIMER,
  BACKGROUND
} body_type_t;

/**
 * Make an info field based on body type.
 *
 * @param type type of body
 * @return pointer to an info
 */
body_type_t *make_type_info(body_type_t type);

/**
 * Get the info associated with the inputted body.
 *
 * @param body pointer to the body
 * @return body type
 */
body_type_t get_type(body_t *body);