#include "../include/body.h"
#include "../include/sdl_wrapper.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Holds the body and direction at which the portal is facing.
 */
typedef struct portal portal_t;

/**
 * Allocates memory for a new portal pointer.
 * 
 * @param body a pointer to the portal body
 * @param direction a unit vector representing the direction at which
 * the portal is facing
 * @return a pointer to the newly allocated portal
 */
portal_t *portal_init(body_t *body, vector_t direction);

/**
 * Free the portal and remove its body.
 *
 * @param portal a pointer to a portal
 */
void portal_free(portal_t *portal);

/**
 * Set the portal's direction to a specified unit vector.
 * 
 * @param portal a pointer to a portal
 * @param direction a unit vector reprsenting the direction at which
 * the portal is facing
 */
void portal_set_direction(portal_t *portal, vector_t direction);

/**
 * Get the pointer to the portal's body.
 * 
 * @param portal a pointer to a portal
 * @return a pointer to the portal's body
 */
body_t *portal_get_body(portal_t *portal);

/**
 * Get the portal's direction.
 * 
 * @param portal a pointer to a portal
 * @return a unit vector reprsenting the direction at which
 * the portal is facing
 */
vector_t portal_get_direction(portal_t *portal);

/**
 * Updates the position and velocity of the transport_body if it
 * teleports through portal to other_portal. 
 * Changes the value of the is_teleporting pointer is pointing to
 * based on if transport_body is teleporting through the portals.
 * Plays sound effect if teleporting through portal.
 * 
 * @param portal a pointer to the portal the transport_body is entering
 * @param other_portal a pointer to the portal the transport_body will exit from
 * @param transport_body a pointer to the body that is teleporting through the portals
 * @param is_teleporting a pointer to whether or not the transport_body is teleporting
 */
void portal_tick(portal_t *portal, portal_t *other_portal,
                 body_t *transport_body, bool *is_teleporting);