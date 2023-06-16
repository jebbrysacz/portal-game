#include "../include/scene.h"
#include "test_util.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

void scene_get_first(void *scene) { scene_get_body(scene, 0); }
void scene_remove_first(void *scene) { scene_remove_body(scene, 0); }

void test_empty_scene() {
  scene_t *scene = scene_init();
  assert(scene_bodies(scene) == 0);
  for (int i = 0; i < 10; i++)
    scene_tick(scene, 1);
  assert(test_assert_fail(scene_get_first, scene));
  assert(test_assert_fail(scene_remove_first, scene));
  scene_free(scene);
}

list_t *make_shape() {
  list_t *shape = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){-1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, +1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-1, +1};
  list_add(shape, v);
  return shape;
}

void test_scene() {
  // Build a scene with 3 bodies
  scene_t *scene = scene_init();
  assert(scene_bodies(scene) == 0);
  body_t *body1 = body_init(make_shape(), 1, (rgb_color_t){1, 1, 1});
  scene_add_body(scene, body1);
  assert(scene_bodies(scene) == 1);
  assert(scene_get_body(scene, 0) == body1);
  body_t *body2 = body_init(make_shape(), 2, (rgb_color_t){1, 1, 1});
  scene_add_body(scene, body2);
  assert(scene_bodies(scene) == 2);
  assert(scene_get_body(scene, 0) == body1);
  assert(scene_get_body(scene, 1) == body2);
  body_t *body3 = body_init(make_shape(), 3, (rgb_color_t){1, 1, 1});
  scene_add_body(scene, body3);
  assert(scene_bodies(scene) == 3);
  assert(scene_get_body(scene, 0) == body1);
  assert(scene_get_body(scene, 1) == body2);
  assert(scene_get_body(scene, 2) == body3);

  // Set the bodies' positions with no velocity and ensure they match
  body_set_centroid(body1, (vector_t){1, 1});
  body_set_centroid(body2, (vector_t){2, 2});
  body_set_centroid(body3, (vector_t){3, 3});
  scene_tick(scene, 1);
  assert(vec_isclose(body_get_centroid(body1), (vector_t){1, 1}));
  assert(vec_isclose(body_get_centroid(body2), (vector_t){2, 2}));
  assert(vec_isclose(body_get_centroid(body3), (vector_t){3, 3}));
  body_set_velocity(body1, (vector_t){+1, 0});
  body_set_velocity(body2, (vector_t){-1, 0});
  body_set_velocity(body3, (vector_t){0, +1});
  scene_tick(scene, 1);
  assert(vec_isclose(body_get_centroid(body1), (vector_t){2, 1}));
  assert(vec_isclose(body_get_centroid(body2), (vector_t){1, 2}));
  assert(vec_isclose(body_get_centroid(body3), (vector_t){3, 4}));

  // Try removing the second body
  scene_remove_body(scene, 1);
  assert(scene_bodies(scene) == 3); // removal is deferred until the next tick
  scene_tick(scene, 0);
  assert(scene_bodies(scene) == 2);
  assert(scene_get_body(scene, 0) == body1);
  assert(scene_get_body(scene, 1) == body3);

  // Tick the remaining bodies
  scene_tick(scene, 1);
  assert(vec_isclose(body_get_centroid(body1), (vector_t){3, 1}));
  assert(vec_isclose(body_get_centroid(body3), (vector_t){3, 5}));

  scene_free(scene);
}

// A force creator that moves a body in uniform circular motion about the origin
void centripetal_force(void *aux) {
  body_t *body = aux;
  vector_t v = body_get_velocity(body);
  vector_t r = body_get_centroid(body);
  assert(isclose(vec_dot(v, r), 0));
  vector_t force =
      vec_multiply(-body_get_mass(body) * vec_dot(v, v) / vec_dot(r, r), r);
  body_add_force(body, force);
}

void test_force_creator() {
  const double OMEGA = 3;
  const double R = 2;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *body = body_init(make_shape(), 123, (rgb_color_t){0, 0, 0});
  vector_t radius = {R, 0};
  body_set_centroid(body, radius);
  body_set_velocity(body, (vector_t){0, OMEGA * R});
  scene_add_body(scene, body);
  scene_add_force_creator(scene, centripetal_force, body, NULL);
  for (int i = 0; i < STEPS; i++) {
    vector_t expected_x = vec_rotate(radius, OMEGA * i * DT);
    assert(vec_within(1e-4, body_get_centroid(body), expected_x));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

typedef struct {
  scene_t *scene;
  double coefficient;
} force_aux_t;

// A force creator that applies constant downwards gravity to all bodies in a
// scene
void constant_gravity(void *aux) {
  force_aux_t *gravity_aux = aux;
  size_t body_count = scene_bodies(gravity_aux->scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(gravity_aux->scene, i);
    vector_t force = {0, -gravity_aux->coefficient * body_get_mass(body)};
    body_add_force(body, force);
  }
}

// A force creator that applies drag proportional to v ** 2 to all bodies in a
// scene
void air_drag(void *aux) {
  force_aux_t *drag_aux = aux;
  size_t body_count = scene_bodies(drag_aux->scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(drag_aux->scene, i);
    vector_t v = body_get_velocity(body);
    vector_t force =
        vec_multiply(-drag_aux->coefficient * sqrt(vec_dot(v, v)), v);
    body_add_force(body, force);
  }
}

void test_force_creator_aux() {
  const double LIGHT_MASS = 10, HEAVY_MASS = 20;
  const double GRAVITY = 9.8, DRAG = 3;
  const double DT = 1e-3;
  const int STEPS = 100000;
  scene_t *scene = scene_init();
  body_t *light = body_init(make_shape(), LIGHT_MASS, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, light);
  body_t *heavy = body_init(make_shape(), HEAVY_MASS, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, heavy);
  force_aux_t *gravity_aux = malloc(sizeof(*gravity_aux));
  gravity_aux->scene = scene;
  gravity_aux->coefficient = GRAVITY;
  scene_add_force_creator(scene, constant_gravity, gravity_aux, free);
  force_aux_t *drag_aux = malloc(sizeof(*drag_aux));
  drag_aux->scene = scene;
  drag_aux->coefficient = DRAG;
  scene_add_force_creator(scene, air_drag, drag_aux, free);
  for (int i = 0; i < STEPS; i++)
    scene_tick(scene, DT);

  assert(vec_isclose(body_get_velocity(light),
                     (vector_t){0, -sqrt(GRAVITY * LIGHT_MASS / DRAG)}));
  assert(vec_isclose(body_get_velocity(heavy),
                     (vector_t){0, -sqrt(GRAVITY * HEAVY_MASS / DRAG)}));
  scene_free(scene);
}

/*
    This test checks that a force creator is no longer called after
    one of its bodies has been removed.
    There are initially 3 bodies in the scene.
    remove_body() removes the last remaining body each tick.
    count_calls() depends on the first and second bodies,
    so it should only be called during the first two ticks.
*/
void remove_body(void *aux) {
  scene_t *scene = aux;
  size_t body_count = scene_bodies(scene);
  if (body_count > 0) {
    body_remove(scene_get_body(scene, body_count - 1));
  }
}
typedef struct {
  int count;
  scene_t *scene;
} count_aux_t;

void count_calls(void *aux) {
  count_aux_t *count_aux = aux;
  // Every time count_calls() is called, the body count should decrease by 1
  assert(scene_bodies(count_aux->scene) == 3 - count_aux->count);
  // Record that count_calls() was called an additional time
  count_aux->count++;
}

void test_reaping() {
  scene_t *scene = scene_init();
  for (int i = 0; i < 3; i++) {
    scene_add_body(scene, body_init(make_shape(), 1, (rgb_color_t){0, 0, 0}));
  }
  scene_add_bodies_force_creator(scene, remove_body, scene, list_init(0, NULL),
                                 NULL);

  count_aux_t *count_aux = malloc(sizeof(*count_aux));
  count_aux->count = 0;
  count_aux->scene = scene;
  list_t *required_bodies = list_init(2, NULL);
  list_add(required_bodies, scene_get_body(scene, 0));
  list_add(required_bodies, scene_get_body(scene, 1));
  scene_add_bodies_force_creator(scene, count_calls, count_aux, required_bodies,
                                 NULL);

  while (scene_bodies(scene) > 0) {
    scene_tick(scene, 1);
  }

  assert(count_aux->count == 2);
  free(count_aux);
  scene_free(scene);
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_empty_scene)
  DO_TEST(test_scene)
  DO_TEST(test_force_creator)
  DO_TEST(test_force_creator_aux)
  DO_TEST(test_reaping)

  puts("scene_test PASS");
}
