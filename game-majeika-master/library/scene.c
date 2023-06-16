#include "../include/scene.h"
#include "../include/body.h"
#include "../include/forces.h"
#include "../include/platform.h"
#include "../include/portal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const size_t INITIAL_NUM_BODIES = 10;
const size_t INITIAL_NUM_FORCE_CREATORS = 10;

typedef struct scene {
  list_t *bodies;
  list_t *force_appliers;
} scene_t;

scene_t *scene_init(void) {
  scene_t *new_scene = calloc(1, sizeof(scene_t));
  assert(new_scene);
  new_scene->bodies = list_init(INITIAL_NUM_BODIES, (free_func_t)body_free);
  new_scene->force_appliers =
      list_init(INITIAL_NUM_FORCE_CREATORS, (free_func_t)force_applier_free);

  return new_scene;
}

void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->force_appliers);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  return (body_t *)list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
  body_remove(scene_get_body(scene, index));
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
  scene_add_bodies_force_creator(scene, forcer, aux, NULL, freer);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  list_add(scene->force_appliers,
           force_applier_init(forcer, aux, bodies, freer));
}

void scene_tick(scene_t *scene, double dt) {
  for (size_t i = 0; i < list_size(scene->force_appliers); i++) {
    force_applier_t *force_applier = list_get(scene->force_appliers, i);
    force_creator_t forcer = get_force_applier_forcer(force_applier);
    void *aux = get_force_applier_aux(force_applier);
    forcer(aux);
  }
  for (size_t i = list_size(scene->bodies); i > 0; i--) {
    body_t *body = list_get(scene->bodies, i - 1);
    if (body_is_removed(body)) {
      for (size_t j = list_size(scene->force_appliers); j > 0; j--) {
        force_applier_t *force_applier = list_get(scene->force_appliers, j - 1);
        list_t *bodies = get_force_applier_bodies(force_applier);
        for (size_t k = 0; k < list_size(bodies); k++) {
          if (list_get(bodies, k) == body) {
            force_applier_free(list_remove(scene->force_appliers, j - 1));
            break;
          }
        }
      }
      body_free(list_remove(scene->bodies, i - 1));
    } else {
      body_tick(body, dt);
    }
  }
}