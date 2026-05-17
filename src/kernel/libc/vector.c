/**
 * Vector - Growable array implementation
 * Uses a dynamically allocated array that grows as needed
 */

#include "vector.h"
#include "../memory/memory.h"
#include "string.h"

// internal helper function to resize the capacity of the vector
static int vector_resize(Vector *vec, size_t new_capacity) {
  // allocate the new memory block
  void *new_data = kmalloc(new_capacity * vec->element_size);
  if (!new_data) {
    return 0; // allocation failed (out of memory)
  }

  // if the is existing datam copy it over to the new block
  if (vec->data && vec->size > 0) {
    // only copy to the new capacity if shrinking (though we mostly grow)
    size_t elements_to_copy =
        (vec->size < new_capacity) ? vec->size : new_capacity;
    memcpy(new_data, vec->data, elements_to_copy * vec->element_size);

    // free the old memory block using your custom allocator
    kfree(vec->data);
  }

  vec->data = new_data;
  vec->capacity = new_capacity;
  return 1;
}

int vector_init(Vector *vec, size_t initial_capacity, size_t element_size) {
  if (!vec || element_size == 0) {
    return 0;
  }

  vec->size = 0;
  vec->capacity = (initial_capacity == 0) ? 4 : initial_capacity;
  vec->element_size = element_size;

  // allocate initial backing array
  vec->data = kmalloc(vec->capacity * vec->element_size);
  if (!vec->data) {
    vec->capacity = 0;
    return 0; // initialization failed
  }

  return 1;
}

void vector_free(Vector *vec) {
  if (vec && vec->data) {
    kfree(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
  }
}

int vector_push_back(Vector *vec, const void *element) {
  if (!vec || !element) {
    return 0;
  }

  // check if we  need to double the capacity
  if (vec->size >= vec->capacity) {
    size_t new_capacity = vec->capacity * 2;
    if (!vector_resize(vec, new_capacity)) {
      return 0; // failed to grow vector
    }
  }

  // calculate destination address using byte arithmetic
  void *target = (void *)((char *)vec->data + (vec->size * vec->element_size));
  memcpy(target, element, vec->element_size);
  vec->size++;

  return 1;
}

void *vector_get(const Vector *vec, size_t index) {
  if (!vec || index >= vec->size) {
    return NULL; // out of bounds or invalid vector
  }

  // calculate pointer offset
  return (void *)((char *)vec->data + (index * vec->element_size));
}

int vector_pop_back(Vector *vec) {
  if (!vec || vec->size == 0) {
    return 0; // nothing to pop
  }

  vec->size--;
  return 1;
}

void vector_clear(Vector *vec) {
  if (vec) {
    vec->size = 0;
  }
}
