#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "../include/forces.h"
#include "../include/test_util.h"

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

double gravity_potential(double G, body_t *body1, body_t *body2) {
  vector_t r = vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
  return -G * body_get_mass(body1) * body_get_mass(body2) / sqrt(vec_dot(r, r));
}
double kinetic_energy(body_t *body) {
  vector_t v = body_get_velocity(body);
  return body_get_mass(body) * vec_dot(v, v) / 2;
}

// tests that a three body system conserves energy
void test_energy_conservation_3_body_system() {
  const double M1 = 8.0, M2 = 3.7, M3 = 5.5;
  const double G = 1e3;
  const double DT = 1e-6;
  const int STEPS = 100000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass1, (vector_t){0, 50});
  scene_add_body(scene, mass1);
  body_t *mass2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass2, (vector_t){10, 20});
  scene_add_body(scene, mass2);
  body_t *mass3 = body_init(make_shape(), M3, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass3, (vector_t){30, 15});
  scene_add_body(scene, mass3);
  create_newtonian_gravity(scene, G, mass1, mass2);
  create_newtonian_gravity(scene, G, mass1, mass3);
  create_newtonian_gravity(scene, G, mass2, mass3);
  double initial_energy = gravity_potential(G, mass1, mass2) +
                          gravity_potential(G, mass1, mass3) +
                          gravity_potential(G, mass2, mass3);
  for (size_t i = 0; i < STEPS; i++) {
    double total_energy = gravity_potential(G, mass1, mass2) +
                          gravity_potential(G, mass1, mass3) +
                          gravity_potential(G, mass2, mass3) +
                          kinetic_energy(mass1) + kinetic_energy(mass2) +
                          kinetic_energy(mass3);
    assert(fabs(initial_energy - total_energy) <= 1e-4);
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

// tests that a body with drag will have energy decreased by a constant ratio
void test_drag_reduces_energy() {
  const double gamma = 0.99;
  const double M1 = 1;
  const double DT = 1e-6;
  const int STEPS = 1000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  body_set_velocity(mass1, (vector_t){10, 0});
  scene_add_body(scene, mass1);
  create_drag(scene, gamma, mass1);
  double initial_energy = kinetic_energy(mass1);
  double last_energy = initial_energy;
  for (size_t i = 0; i < STEPS; i++) {
    double energy = kinetic_energy(mass1);
    assert(within(1e-4, energy / last_energy, 1.0 - 2e-5));
    last_energy = energy;
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

// test that velocity is correct in a spring and mass system
void test_spring_velocity() {
  const double M = 10;
  const double K = 2;
  const double A = 3;
  const double DT = 1e-6;
  const int STEPS = 100000;
  scene_t *scene = scene_init();
  body_t *mass = body_init(make_shape(), M, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass, (vector_t){A, 0});
  scene_add_body(scene, mass);
  body_t *anchor = body_init(make_shape(), INFINITY, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, anchor);
  create_spring(scene, K, mass, anchor);
  for (int i = 0; i < STEPS; i++) {
    assert(vec_equal(body_get_centroid(anchor), VEC_ZERO));
    assert(vec_isclose(
        body_get_velocity(mass),
        (vector_t){-1 * A * sqrt(K / M) * sin(sqrt(K / M) * i * DT), 0.0}));
    scene_tick(scene, DT);
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

  DO_TEST(test_energy_conservation_3_body_system)
  DO_TEST(test_drag_reduces_energy)
  DO_TEST(test_spring_velocity)

  puts("student_forces_test PASS");
}