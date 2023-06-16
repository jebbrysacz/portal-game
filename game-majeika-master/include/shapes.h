#include "../include/list.h"
#include <math.h>

/**
 * Convert an angle from degrees to radians.
 *
 * @param deg angle in degrees
 * @return angle in radians
 */
double deg_to_rad(double deg);

/**
 * Create a list of points for a rectangular body.
 *
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @return a list of points
 */
list_t *make_rect_shape(double width, double height);

/**
 * Create a list of points for a circular body.
 *
 * @param radius radius of the circle
 * @param num_points number of points in list; i.e. resolution of circle
 * @return a list of points
 */
list_t *make_circ_shape(double radius, size_t num_points);
