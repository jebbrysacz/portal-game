#include "../include/forces.h"
#include "test_util.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

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

// Tests that a mass on a spring oscillates like A cos(sqrt(K / M) * t)
void test_spring_sinusoid() {
  const double M = 10;
  const double K = 2;
  const double A = 3;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *mass = body_init(make_shape(), M, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass, (vector_t){A, 0});
  scene_add_body(scene, mass);
  body_t *anchor = body_init(make_shape(), INFINITY, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, anchor);
  create_spring(scene, K, mass, anchor);
  for (int i = 0; i < STEPS; i++) {
    assert(vec_isclose(body_get_centroid(mass),
                       (vector_t){A * cos(sqrt(K / M) * i * DT), 0}));
    assert(vec_equal(body_get_centroid(anchor), VEC_ZERO));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

double gravity_potential(double G, body_t *body1, body_t *body2) {
  vector_t r = vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
  return -G * body_get_mass(body1) * body_get_mass(body2) / sqrt(vec_dot(r, r));
}
double kinetic_energy(body_t *body) {
  vector_t v = body_get_velocity(body);
  return body_get_mass(body) * vec_dot(v, v) / 2;
}

// Tests that a conservative force (gravity) conserves K + U
void test_energy_conservation() {
  const double M1 = 4.5, M2 = 7.3;
  const double G = 1e3;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, mass1);
  body_t *mass2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass2, (vector_t){10, 20});
  scene_add_body(scene, mass2);
  create_newtonian_gravity(scene, G, mass1, mass2);
  double initial_energy = gravity_potential(G, mass1, mass2);
  for (int i = 0; i < STEPS; i++) {
    assert(body_get_centroid(mass1).x < body_get_centroid(mass2).x);
    double energy = gravity_potential(G, mass1, mass2) + kinetic_energy(mass1) +
                    kinetic_energy(mass2);
    assert(within(1e-4, energy / initial_energy, 1.0));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

body_t *make_triangle_body() {
  list_t *shape = list_init(3, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){1, 0};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-0.5, +sqrt(3) / 2};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-0.5, -sqrt(3) / 2};
  list_add(shape, v);
  return body_init(shape, 1, (rgb_color_t){0, 0, 0});
}

// Tests that destructive collisions remove bodies from the scene
void test_collisions() {
  const double DT = 0.1;
  const double V = 1.23;
  const double SEPARATION_AT_COLLISION = 1.5;
  const int TICKS_TO_COLLISION = 10;

  scene_t *scene = scene_init();
  body_t *body1 = make_triangle_body();
  vector_t initial_separation = {
      SEPARATION_AT_COLLISION + V * DT * (TICKS_TO_COLLISION - 0.5), 0};
  body_set_centroid(body1, vec_negate(initial_separation));
  body_set_velocity(body1, (vector_t){+V, 0});
  scene_add_body(scene, body1);

  body_t *body2 = make_triangle_body();
  scene_add_body(scene, body2);

  body_t *body3 = make_triangle_body();
  body_set_velocity(body3, (vector_t){-V, 0});
  body_set_centroid(body3, initial_separation);
  scene_add_body(scene, body3);

  create_destructive_collision(scene, body1, body2);
  create_destructive_collision(scene, body1, body3);
  create_destructive_collision(scene, body2, body3);
  for (int i = 0; i < TICKS_TO_COLLISION * 2; i++) {
    scene_tick(scene, DT);
    // Before collision, there should be 2 bodies; after, there should be 0
    if (i < TICKS_TO_COLLISION) {
      assert(scene_bodies(scene) == 3);
    } else {
      assert(scene_bodies(scene) == 0);
    }
  }
  scene_free(scene);
}

// Tests that force creators properly register their list of affected bodies.
// If they don't, asan will report a heap-use-after-free failure.
void test_forces_removed() {
  scene_t *scene = scene_init();
  for (int i = 0; i < 10; i++) {
    body_t *body = body_init(make_shape(), 1, (rgb_color_t){0, 0, 0});
    body_set_centroid(body, (vector_t){i, i});
    scene_add_body(scene, body);
    for (int j = 0; j < i; j++) {
      create_newtonian_gravity(scene, 1, body, scene_get_body(scene, j));
      create_spring(scene, 1, body, scene_get_body(scene, j));
    }
    create_drag(scene, 1, body);
  }
  while (scene_bodies(scene) > 0) {
    scene_remove_body(scene, 0);
    scene_tick(scene, 1);
  }
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

  DO_TEST(test_spring_sinusoid)
  DO_TEST(test_energy_conservation)
  DO_TEST(test_collisions)
  DO_TEST(test_forces_removed)

  puts("forces_test PASS");
}
