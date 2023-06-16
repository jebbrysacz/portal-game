#include "../include/portal.h"
#include "../include/collision.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

const double PORTAL_MOVE_CONST = 20;
const char *PORTAL_SOUND_EFFECT_PATH = "assets/sounds/portal.wav";

typedef struct portal {
  body_t *body;
  vector_t direction;
} portal_t;

portal_t *portal_init(body_t *body, vector_t direction) {
  portal_t *new_portal = calloc(1, sizeof(portal_t));
  assert(new_portal);
  new_portal->body = body;
  new_portal->direction = direction;

  return new_portal;
}

void portal_free(portal_t *portal) {
  body_remove(portal->body);
  free(portal);
}

void portal_set_direction(portal_t *portal, vector_t direction) {
  portal->direction = direction;
}

body_t *portal_get_body(portal_t *portal) { return portal->body; }

vector_t portal_get_direction(portal_t *portal) { return portal->direction; }

void portal_tick(portal_t *portal, portal_t *other_portal,
                 body_t *transport_body, bool *is_teleporting) {
  if (portal && other_portal && transport_body) {
    vector_t dir1 = portal->direction;
    vector_t dir2 = other_portal->direction;

    vector_t portal_centroid = body_get_centroid(portal->body);
    vector_t other_portal_centroid = body_get_centroid(other_portal->body);
    vector_t transport_centroid = body_get_centroid(transport_body);

    vector_t direction_vec = vec_subtract(transport_centroid, portal_centroid);

    list_t *portal_shape = body_get_shape(portal->body);
    list_t *other_portal_shape = body_get_shape(other_portal->body);
    list_t *transport_shape = body_get_shape(transport_body);

    collision_info_t collision_info =
        find_collision(portal_shape, transport_shape);
    collision_info_t collision_info_other =
        find_collision(other_portal_shape, transport_shape);
    list_free(portal_shape);
    list_free(transport_shape);
    list_free(other_portal_shape);

    if (collision_info.collided || collision_info_other.collided) {
      *is_teleporting = true;
    } else {
      *is_teleporting = false;
    }

    // How much the transport_body has gone through a plane orthogonal to portal.
    // overlap > 0: transport_body has not gone through portal
    // overlap = 0: transport_body on same plane as portal
    // overlap < 0: transport_body has gone through portal
    double overlap = vec_dot(dir1, direction_vec);
    if (collision_info.collided && overlap <= 0) {
      vector_t new_centroid =
          vec_add(other_portal_centroid, vec_multiply(PORTAL_MOVE_CONST, dir2));
      vector_t new_velocity = vec_negate(
          vec_rotate(body_get_velocity(transport_body),
                     vec_direction_angle(dir2) - vec_direction_angle(dir1)));
                     
      body_set_centroid(transport_body, new_centroid);
      body_set_velocity(transport_body, new_velocity);
      
      sdl_play_sound(PORTAL_SOUND_EFFECT_PATH);
    }
  }
}