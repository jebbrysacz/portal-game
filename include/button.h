#include "../include/body.h"

/**
 * A body that, when an appropriate body is placed on it,
 * will trigger a number of platforms to move. 
 */
typedef struct button button_t;

/**
 * Allocates memory for a pointer to a button.
 *
 * @param pos a vector describing the button's position. pos.x is the center and
 * pos.y is the bottom-most point of the button.
 * @param button_dims a vector describing the button's x and y dimensions
 * @param button_color the button's color
 * @param base_dims a vector describing the button base's x and y dimensions
 * @param base_color the base's color
 * @param platforms a pointer to the list of platforms that are controlled by a button
 * @return a pointer to the newly allocated button
 */
button_t *button_init(vector_t pos, vector_t button_dims,
                      rgb_color_t button_color, vector_t base_dims,
                      rgb_color_t base_color, list_t *platforms);

/**
 * Free the button and contents within.
 *
 * @param button a pointer to a button
 */
void button_free(button_t *button);

/**
 * Return the body from the button struct
 *
 * @param button a pointer to a button
 * @return the pointer to the body of the button
 */
body_t *button_get_button_body(button_t *button);

/**
 * Returns the body of the button's base
 *
 * @param button a pointer to a button
 * @return a pointer to the body of the button
 */
body_t *button_get_base_body(button_t *button);

/**
 * Complete the button animation and
 * activate the corresponding platforms when pressed 
 *
 * @param button the pointer to the button struct
 * @param pressing_bodies the pointer to the list of bodies that can be on the button
 * @param dt the number of seconds elapsed since the last tick
 */
void button_tick(button_t *button, list_t *pressing_bodies, double dt);