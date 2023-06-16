#include "../include/forces.h"
#include "../include/collision.h"
#include "../include/scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double MINIMUM_DISTANCE = 5;
const double GRAVITY_FORCE_THRESHOLD = 1e2;

typedef struct force_aux {
  double force_constant;
  body_t *body1;
  body_t *body2;
  collision_handler_t collision_handler;
  void *aux;
  free_func_t freer;
  bool collided_last_tick;
} force_aux_t;

force_aux_t *force_aux_init(double force_constant, body_t *body1, body_t *body2,
                            collision_handler_t collision_handler, void *aux,
                            free_func_t freer, bool collided_last_tick) {
  force_aux_t *force_aux = calloc(1, sizeof(force_aux_t));
  assert(force_aux);
  force_aux->force_constant = force_constant;
  force_aux->body1 = body1;
  force_aux->body2 = body2;
  force_aux->collision_handler = collision_handler;
  force_aux->aux = aux;
  force_aux->freer = freer;
  force_aux->collided_last_tick = collided_last_tick;

  return force_aux;
}

void force_aux_free(force_aux_t *force_aux) {
  if (force_aux->aux && force_aux->freer) {
    force_aux->freer(force_aux->aux);
  }
  free(force_aux);
}

typedef struct force_applier {
  force_creator_t forcer;
  void *aux;
  list_t *bodies;
  free_func_t freer;
} force_applier_t;

force_applier_t *force_applier_init(force_creator_t forcer, void *aux,
                                    list_t *bodies, free_func_t freer) {
  force_applier_t *force_applier = calloc(1, sizeof(force_applier_t));
  force_applier->forcer = forcer;
  force_applier->aux = aux;
  force_applier->freer = freer;
  force_applier->bodies = bodies;

  return force_applier;
}

force_creator_t get_force_applier_forcer(force_applier_t *force_applier) {
  return force_applier->forcer;
}

void *get_force_applier_aux(force_applier_t *force_applier) {
  return force_applier->aux;
}

list_t *get_force_applier_bodies(force_applier_t *force_applier) {
  return force_applier->bodies;
}

void force_applier_free(force_applier_t *force_applier) {
  force_aux_t *force_aux = force_applier->aux;
  list_t *bodies = force_applier->bodies;
  if (bodies) {
    list_free(bodies);
  }
  if (force_aux && force_applier->freer) {
    force_applier->freer(force_aux);
  }
  free(force_applier);
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
  force_aux_t *force_aux =
      force_aux_init(G, body1, body2, NULL, NULL, NULL, false);

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(
      scene, (force_creator_t)apply_newtonian_gravity, force_aux, bodies,
      (free_func_t)force_aux_free);
}

void apply_newtonian_gravity(void *aux) {
  force_aux_t *force_aux = aux;
  double G = force_aux->force_constant;
  body_t *body1 = force_aux->body1;
  body_t *body2 = force_aux->body2;
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  double mass1 = body_get_mass(body1);
  double mass2 = body_get_mass(body2);

  // Displacement and distance between the two centroids
  vector_t displacement = vec_subtract(centroid2, centroid1);
  double distance = sqrt(vec_dot(displacement, displacement));
  if (distance >= MINIMUM_DISTANCE) {
    // Unit vector for direction of force
    vector_t unit_vector = vec_multiply(1 / distance, displacement);
    double force_magnitude = G * mass1 * mass2 / pow(distance, 2);
    vector_t gravity_force = vec_multiply(force_magnitude, unit_vector);

    if (fabs(gravity_force.x) < GRAVITY_FORCE_THRESHOLD) {
      gravity_force.x = 0;
    }
    if (fabs(gravity_force.y) < GRAVITY_FORCE_THRESHOLD) {
      gravity_force.y = 0;
    }

    body_add_force(body1, gravity_force);
    body_add_force(body2, vec_negate(gravity_force));
  }
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
  force_aux_t *force_aux =
      force_aux_init(k, body1, body2, NULL, NULL, NULL, false);

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_spring,
                                 force_aux, bodies,
                                 (free_func_t)force_aux_free);
}

void apply_spring(void *aux) {
  force_aux_t *force_aux = aux;
  double k = force_aux->force_constant;
  body_t *body1 = force_aux->body1;
  body_t *body2 = force_aux->body2;
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);

  vector_t displacement = vec_subtract(centroid2, centroid1);
  vector_t spring_force = vec_multiply(k, displacement);
  body_add_force(body1, spring_force);
  body_add_force(body2, vec_negate(spring_force));
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
  force_aux_t *force_aux =
      force_aux_init(gamma, body, NULL, NULL, NULL, NULL, false);

  list_t *bodies = list_init(1, NULL);
  list_add(bodies, body);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_drag, force_aux,
                                 bodies, (free_func_t)force_aux_free);
}

void apply_drag(void *aux) {
  force_aux_t *force_aux = aux;
  double gamma = force_aux->force_constant;
  body_t *body = force_aux->body1;
  vector_t body_vel = body_get_velocity(body);

  vector_t drag_force = vec_multiply(-gamma, body_vel);
  body_add_force(body, drag_force);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  force_aux_t *force_aux =
      force_aux_init(0.0, body1, body2, handler, aux, freer, false);

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_collision,
                                 force_aux, bodies,
                                 (free_func_t)force_aux_free);
}

void apply_collision(void *aux) {
  force_aux_t *force_aux = aux;
  body_t *body1 = force_aux->body1;
  body_t *body2 = force_aux->body2;
  collision_handler_t handler = force_aux->collision_handler;
  void *aux_ = (void *)force_aux->aux;

  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  collision_info_t collision_info = find_collision(shape1, shape2);

  list_free(shape1);
  list_free(shape2);

  if (collision_info.collided && !force_aux->collided_last_tick) {
    handler(body1, body2, collision_info.axis, aux_);
    force_aux->collided_last_tick = true;
  } else if (collision_info.collided) {
    force_aux->collided_last_tick = true;
  } else {
    force_aux->collided_last_tick = false;
  }
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  create_collision(scene, body1, body2,
                   (collision_handler_t)destructive_collision_handler, NULL,
                   NULL);
}

void destructive_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                                   void *aux) {
  body_remove(body1);
  body_remove(body2);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  double *aux = calloc(1, sizeof(double));
  assert(aux);
  *aux = elasticity;
  create_collision(scene, body1, body2,
                   (collision_handler_t)physics_collision_handler, aux, free);
}

void physics_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                               void *aux) {
  double elasticity = *(double *)aux;
  double mass1 = body_get_mass(body1);
  double mass2 = body_get_mass(body2);
  vector_t v_1 = body_get_velocity(body1);
  vector_t v_2 = body_get_velocity(body2);

  double reduced_mass;
  if (mass1 == INFINITY) {
    reduced_mass = mass2;
  } else if (mass2 == INFINITY) {
    reduced_mass = mass1;
  } else {
    reduced_mass = mass1 * mass2 / (mass1 + mass2);
  }

  double u_1 = vec_dot(v_1, axis);
  double u_2 = vec_dot(v_2, axis);
  double J_n = reduced_mass * (1 + elasticity) * (u_2 - u_1);

  vector_t impulse = vec_multiply(J_n, axis);

  body_add_impulse(body1, impulse);
  body_add_impulse(body2, vec_negate(impulse));
}

void create_physics_portal_collision(scene_t *scene, body_t *moving_body,
                                     body_t *stationary_body,
                                     bool *is_teleporting) {
  create_collision(scene, moving_body, stationary_body,
                   (collision_handler_t)apply_physics_portal_collision_handler,
                   is_teleporting, NULL);
}

void apply_physics_portal_collision_handler(body_t *body1, body_t *body2,
                                            vector_t axis, void *aux) {
  body_t *moving_body = body1;
  body_t *stationary_body = body2;
  bool *is_teleporting = aux;
  double elasticity = 0;
  double moving_body_mass = body_get_mass(body1);
  vector_t v_1 = body_get_velocity(body1);
  vector_t v_2 = body_get_velocity(body2);

  // Only run if there is no portal on stationary body
  if (!(*is_teleporting)) {
    // Stationary body is infinity so reduced_mass is automatically
    // moving_body_mass
    double reduced_mass = moving_body_mass;

    double u_1 = vec_dot(v_1, axis);
    double u_2 = vec_dot(v_2, axis);
    double J_n = reduced_mass * (1 + elasticity) * (u_2 - u_1);

    vector_t impulse = vec_multiply(J_n, axis);

    body_add_impulse(moving_body, impulse);
    body_add_impulse(stationary_body, vec_negate(impulse));
  }
}

void create_normal_force(scene_t *scene, body_t *body1, body_t *body2,
                         bool *is_teleporting) {
  force_aux_t *force_aux =
      force_aux_init(0, body1, body2, NULL, is_teleporting, NULL, false);

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_normal_force,
                                 force_aux, bodies,
                                 (free_func_t)force_aux_free);
}

void apply_normal_force(void *aux) {
  force_aux_t *force_aux = aux;
  body_t *body1 = force_aux->body1;
  body_t *body2 = force_aux->body2;
  bool *is_teleporting = force_aux->aux;

  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);

  collision_info_t collision_info = find_collision(shape1, shape2);
  vector_t axis = collision_info.axis;

  bool can_apply_normal_force = false;
  if (is_teleporting == NULL) {
    can_apply_normal_force = collision_info.collided;
  } else {
    can_apply_normal_force = collision_info.collided && !(*is_teleporting);
  }

  if (can_apply_normal_force) {
    // Negate collision axis if needed
    vector_t direction_vec =
        vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
    if (vec_dot(direction_vec, axis) < 0) {
      axis = vec_negate(axis);
    }

    // Find net force on each body along collision axis
    double net_force_1_along_axis = vec_dot(body_get_force(body1), axis);
    double net_force_2_along_axis =
        vec_dot(body_get_force(body2), vec_negate(axis));

    if (net_force_1_along_axis < 0) {
      net_force_1_along_axis = 0;
    }
    if (net_force_2_along_axis < 0) {
      net_force_2_along_axis = 0;
    }

    // Calculate normal forces
    vector_t normal_force_1 =
        vec_multiply(net_force_1_along_axis, vec_negate(axis));
    vector_t normal_force_2 = vec_multiply(net_force_2_along_axis, axis);

    // Apply normal forces
    body_add_force(body1, normal_force_1);
    body_add_force(body2, normal_force_2);
  }

  list_free(shape1);
  list_free(shape2);
}

void create_jump_force(scene_t *scene, double jump_speed, body_t *jump_body,
                       body_t *stationary_body, bool *is_jumping) {
  force_aux_t *force_aux = force_aux_init(
      jump_speed, jump_body, stationary_body, NULL, is_jumping, NULL, false);

  list_t *bodies = list_init(2, NULL);
  list_add(bodies, jump_body);
  list_add(bodies, stationary_body);
  scene_add_bodies_force_creator(scene, (force_creator_t)apply_jump_force,
                                 force_aux, bodies,
                                 (free_func_t)force_aux_free);
}

void apply_jump_force(void *aux) {
  force_aux_t *force_aux = aux;
  double jump_speed = force_aux->force_constant;
  bool *is_jumping = force_aux->aux;
  body_t *jump_body = force_aux->body1;
  body_t *stationary_body = force_aux->body2;
  list_t *shape_jump = body_get_shape(jump_body);
  list_t *shape_stationary = body_get_shape(stationary_body);
  vector_t centroid_jump = body_get_centroid(jump_body);
  vector_t centroid_stationary = body_get_centroid(stationary_body);

  collision_info_t collision_info =
      find_collision(shape_jump, shape_stationary);

  // Jump only when colliding, jumping, and when moving body above stationary
  if (collision_info.collided && *is_jumping &&
      centroid_jump.y >= centroid_stationary.y) {
    vector_t new_velocity = {body_get_velocity(jump_body).x, jump_speed};
    body_set_velocity(jump_body, new_velocity);
    *is_jumping = false;
  }
}