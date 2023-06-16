#include "../include/list.h"
#include "../include/vector.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int GROWTH_FACTOR = 2;

typedef struct list {
  void **arr;
  size_t size;
  size_t capacity;
  free_func_t freer;
} list_t;

list_t *list_init(size_t capacity, free_func_t freer) {
  list_t *new_list = calloc(1, sizeof(list_t));
  assert(new_list);
  if (capacity < 1) {
    capacity = 1;
  }
  new_list->arr = calloc(capacity, sizeof(void *));
  assert(new_list->arr);
  new_list->size = 0;
  new_list->capacity = capacity;
  new_list->freer = freer;
  return new_list;
}

size_t list_size(list_t *lst) { return lst->size; }

void list_free(list_t *lst) {
  if (lst->freer) {
    for (size_t i = 0; i < list_size(lst); i++) {
      lst->freer(lst->arr[i]);
    }
  }
  free(lst->arr);
  free(lst);
}

void *list_get(list_t *lst, size_t index) {
  assert(index < list_size(lst));
  return lst->arr[index];
}

void list_set(list_t *lst, void *value, size_t index) {
  assert(index < list_size(lst));
  lst->arr[index] = value;
}

void list_ensure_capacity(list_t *lst) {
  if (list_size(lst) == lst->capacity) {
    size_t new_capacity = list_size(lst) * GROWTH_FACTOR;
    void **new_arr = malloc(sizeof(void *) * new_capacity);
    assert(new_arr);
    memcpy(new_arr, lst->arr, sizeof(void *) * list_size(lst));
    free(lst->arr);
    lst->arr = new_arr;
    lst->capacity = new_capacity;
  }
}

void list_add(list_t *lst, void *value) {
  list_ensure_capacity(lst);
  assert(list_size(lst) < lst->capacity);
  assert(value);
  lst->arr[list_size(lst)] = value;
  lst->size += 1;
}

void *list_remove(list_t *lst, size_t index) {
  assert(list_size(lst) > 0);
  void *output = lst->arr[index];
  for (size_t i = index; i < list_size(lst) - 1; i++) {
    lst->arr[i] = lst->arr[i + 1];
  }
  lst->size -= 1;
  return output;
}
