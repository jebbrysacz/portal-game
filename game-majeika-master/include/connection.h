#include "../include/body.h"


/**
 * A system of two bodies that will be connected together.
 * When connected, these two bodies will have the same 
 * centroid plus displacement.
 */
typedef struct connection connection_t;

/**
 * Allocates memory for a pointer to the connection struct.
 *
 * @param body a pointer to a body that will have a body connection
 * @param connected_body a pointer to a body that will be connected to another body
 * @param is_connected a boolean represent whether or the two bodies are currently connected
 * @param displacement a vector describing the displacement maintained between the two connected bodies
 * @return a pointer to a connection struct
 */
connection_t *connection_init(body_t *body, body_t *connected_body,
                              bool is_connected, vector_t displacement);

/**
 * Return the body that has a body connection from the connection struct.
 *
 * @param connection a pointer to a connection struct
 * @return a pointer to the body that has a body connection
 */ 
body_t *connection_get_body(connection_t *connection);

/**
 * Return the body that is connected to another body from the connection struct.
 *
 * @param connection a pointer to a connection struct
 * @return a pointer to the body that is conneted with another body
 */ 
body_t *connection_get_connected_body(connection_t *connection);

/**
 * Return a boolean value that check whether two bodies are being connected.
 *
 * @param connection a pointer to a connection struct
 * @return a boolean value describing whether two bodies are being connected
 */ 
bool connection_get_is_connected(connection_t *connection);

/**
 * Return the displacement between the two connected bodies.
 *
 * @param connection a pointer to a connection struct
 * @return a vector describing the displacement between the two connected bodies
 */ 
vector_t connection_get_displacement(connection_t *connection);

/**
 * Turns the connection on.
 *
 * @param connection a pointer to a connection struct
 */
void connection_connect(connection_t *connection);

/**
 * Turns the connection off
 *
 * @param connection a pointer to a connection struct
 */
void connection_disconnect(connection_t *connection);

/**
 * Turns the connection on/off.
 *
 * @param connection a pointer to a connection struct
 */
void connection_toggle(connection_t *connection);

/**
 * Sets the rotation between the body and its connected body.
 *
 * @param connection a pointer to a connection struct
 * @param angle a double representing the desired angle between the connected bodies
 * @param rotate_around_center boolean determining whether to rotate the body around its center
 */
void connection_set_rotation(connection_t *connection, double angle,
                             bool rotate_around_center);