#include "../include/body.h"

/**
 * A rigid body that can rotate and translate in a 
 * given time around a specified point.
 */
typedef struct platform platform_t;

/**
 * Allocates memory for a pointer to a platform.
 *
 * @param body a pointer to a body describing the platform
 * @param total_motion_time a number describing the total time a platform will move when triggered
 * @param total_motion_angle a number describing the total angle a platform moves through
 * @param total_motion_translation a vector describing the line along which the platform is translated
 * @param point_of_rotation a vector describing the point the platform is rotated around
 * @return a pointer to a platform struct
 */
platform_t *platform_init(body_t *body, double total_motion_time,
                          double total_motion_angle,
                          vector_t total_motion_translation,
                          vector_t point_of_rotation);

/**
 * Describes if a platform has been triggered and is moving.
 *
 * @param platform a pointer to a platform struct
 * @return the boolean describing if the platform is moving
 */
bool platform_get_is_moving(platform_t *platform);

/**
 * Toggles the boolean describing if the platform is moving.
 *
 * @param platform a pointer to a platform struct
 * @param is_moving a boolean representing if a platform is moving
 */
void platform_change_is_moving(platform_t *platform, bool is_moving);

/**
 * Executes a tick of a given platform over a small time interval.
 * This will check if a platform is being triggered and will set
 * its movement state when appropriate.
 *
 * @param platform a pointer to a platform struct
 * @param dt the number of seconds elapsed since the last tick
 */
void platform_tick(platform_t *platform, double dt);