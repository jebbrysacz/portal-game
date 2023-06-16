#include "connection.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct connection {
  body_t *body;
  body_t *connected_body;
  bool is_connected;
  vector_t displacement;
} connection_t;

connection_t *connection_init(body_t *body, body_t *connected_body,
                              bool is_connected, vector_t displacement) {
  connection_t *connection = calloc(1, sizeof(connection_t));
  connection->body = body;
  connection->connected_body = connected_body;
  connection->is_connected = is_connected;
  connection->displacement = displacement;

  return connection;
}

body_t *connection_get_body(connection_t *connection) {
  return connection->body;
}

body_t *connection_get_connected_body(connection_t *connection) {
  return connection->connected_body;
}

bool connection_get_is_connected(connection_t *connection) {
  return connection->is_connected;
}

vector_t connection_get_displacement(connection_t *connection) {
  return connection->displacement;
}

void connection_toggle(connection_t *connection) {
  connection->is_connected = !connection->is_connected;

  // Reset body forces, impulses, and velocity
  body_add_force(connection->connected_body,
                 vec_negate(body_get_force(connection->connected_body)));
  body_add_impulse(connection->connected_body,
                   vec_negate(body_get_impulse(connection->connected_body)));
  body_set_velocity(connection->connected_body, (vector_t){0, 0});
}

void connection_set_rotation(connection_t *connection, double angle,
                             bool rotate_around_center) {
  if (connection->is_connected) {
    // Rotate if need to
    if (rotate_around_center) {
      // Calculate change in body angle
      double rotation_angle =
          angle - body_get_rotation(connection->connected_body);
      body_set_rotation(connection->connected_body, rotation_angle);
    }

    double distance =
        sqrt(vec_dot(connection->displacement, connection->displacement));
    vector_t new_pos =
        vec_add(body_get_centroid(connection->body),
                (vector_t){distance * cos(angle), distance * sin(angle)});
    body_set_centroid(connection->connected_body, new_pos);
  }
}