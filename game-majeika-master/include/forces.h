#ifndef __FORCES_H__
#define __FORCES_H__

#include "scene.h"

/**
 * A function called when a collision occurs.
 * @param body1 the first body passed to create_collision()
 * @param body2 the second body passed to create_collision()
 * @param axis a unit vector pointing from body1 towards body2
 *   that defines the direction the two bodies are colliding in
 * @param aux the auxiliary value passed to create_collision()
 */
typedef void (*collision_handler_t)(body_t *body1, body_t *body2, vector_t axis,
                                    void *aux);

typedef struct force_applier force_applier_t;

force_applier_t *force_applier_init(force_creator_t forcer, void *aux,
                                    list_t *bodies, free_func_t freer);

force_creator_t get_force_applier_forcer(force_applier_t *force_applier);

void *get_force_applier_aux(force_applier_t *force_applier);

list_t *get_force_applier_bodies(force_applier_t *force_applier);

void force_applier_free(force_applier_t *force_applier);

/**
 * Adds a force creator to a scene that applies gravity between two bodies.
 * The force creator will be called each tick
 * to compute the Newtonian gravitational force between the bodies.
 * See
 * https://en.wikipedia.org/wiki/Newton%27s_law_of_universal_gravitation#Vector_form.
 * The force should not be applied when the bodies are very close,
 * because its magnitude blows up as the distance between the bodies goes to 0.
 *
 * @param scene the scene containing the bodies
 * @param G the gravitational proportionality constant
 * @param body1 the first body
 * @param body2 the second body
 */
void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2);

/**
 * Applies gravitational force on bodies stored in aux.
 *
 * @param aux an auxiliary value holding bodies and gravitational constant.
 */
void apply_newtonian_gravity(void *aux);

/**
 * Adds a force creator to a scene that acts like a spring between two bodies.
 * The force creator will be called each tick
 * to compute the Hooke's-Law spring force between the bodies.
 * See https://en.wikipedia.org/wiki/Hooke%27s_law.
 *
 * @param scene the scene containing the bodies
 * @param k the Hooke's constant for the spring
 * @param body1 the first body
 * @param body2 the second body
 */
void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2);

/*
 * Applies spring force to all the bodies stored in aux.
 *
 * @param aux an auxillary value holding bodies and spring constant
 */
void apply_spring(void *aux);

/**
 * Adds a force creator to a scene that applies a drag force on a body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its velocity.
 * The force points opposite the body's velocity.
 *
 * @param scene the scene containing the bodies
 * @param gamma the proportionality constant between force and velocity
 *   (higher gamma means more drag)
 * @param body the body to slow down
 */
void create_drag(scene_t *scene, double gamma, body_t *body);

/**
 * Applies the drag force to the body stored in aux
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_drag(void *aux);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies collide.
 * This generalizes create_destructive_collision() from last week,
 * allowing different things to happen on a collision.
 * The handler is passed the bodies, the collision axis, and an auxiliary value.
 * It should only be called once while the bodies are still colliding.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer);

/**
 * Applies the collision handler on the bodies stored in aux.
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_collision(void *aux);

/**
 * Adds a force creator to a scene that destroys two bodies when they collide.
 * The bodies should be destroyed by calling body_remove().
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 */
void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2);

/**
 * Applies the destructive collision on the bodies.
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void destructive_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                                   void *aux);

/**
 * Adds a force creator to a scene that applies impulses
 * to resolve collisions between two bodies in the scene.
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * @param scene the scene containing the bodies
 * @param elasticity the "coefficient of restitution" of the collision;
 * 0 is a perfectly inelastic collision and 1 is a perfectly elastic collision
 * @param body1 the first body
 * @param body2 the second body
 */
void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2);

/**
 * Applies the physics collision on the bodies..
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void physics_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                               void *aux);

/**
 * Works the same as physics_collision but will only apply impulse 
 * if moving_body is not teleporting.
 *
 * @param scene the scene containing the bodies
 * @param moving_body a pointer to the moving body
 * @param stationary_body a pointer to the stationary body
 * @param is_teleporting a pointer to whether or not moving_body is teleporting
 * through portals.
 */
void create_physics_portal_collision(scene_t *scene, body_t *moving_body,
                                     body_t *stationary_body,
                                     bool *is_teleporting);
/**
 * Applies the physics portal collision on the moving_body (body1).
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_physics_portal_collision_handler(body_t *body1, body_t *body2,
                                            vector_t axis, void *aux);

/**
 * Adds a force creator to a scene that applies a normal force between 
 * body1 and body2.
 * The force creator will be called each tick to compute the normal force
 * based on the net force of the two bodies along the collision axis
 * of the bodies.
 *
 * @param scene the scene containing the bodies
 * @param body1 a pointer to a body
 * @param body2 a pointer to a body
 * @param is_teleporting whether or not a body is teleporting in the game
 */
void create_normal_force(scene_t *scene, body_t *body1, body_t *body2,
                         bool *is_teleporting);

/**
 * Applies the normal force to the bodies stored in aux.
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_normal_force(void *aux);

/**
 * Adds a force creator to a scene that applies a jump force between 
 * on jump_body as it jumps from stationary_body.
 * The force creator will be called each tick to decide whether to jump.
 *
 * @param scene the scene containing the bodies
 * @param jump_speed the speed of the jump_body when you initially make it jump
 * @param jump_body a pointer to the jumping body; has finite mass
 * @param stationary_body a pointer to the stationary body; has infinite mass
 * @param is_jumping whether or not the jump_body is jumpong in the game
 */
void create_jump_force(scene_t *scene, double jump_speed,
                       body_t *jump_body, body_t *stationary_body,
                       bool *is_jumping);

/**
 * Applies the jump force to the jump_body stored in aux.
 *
 * @param aux a pointer to an auxiliary variable containing the necessary
 * bodies and constants
 */
void apply_jump_force(void *aux);

#endif // #ifndef __FORCES_H__
