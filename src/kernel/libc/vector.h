#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef struct {
  void *data;          // pointer to the dynamically allocated array
  size_t size;         // number of the elements currently in the vector
  size_t capacity;     // total allocated element slots
  size_t element_size; // size of a single element in bytes
} Vector;

// initialize a new vector with an initial capacity
int vector_init(Vector *vec, size_t initial_capacity, size_t element_size);

// frees the internal memory used by the vector
void vector_free(Vector *vec);

// pushes a new item onto the end of the vector (automatically resizes if full)
int vector_push_back(Vector *vec, const void *element);

// retrieves a pointer to the element at the specified index
void *vector_get(const Vector *vec, size_t index);

// removes the last element from the vector
int vector_pop_back(Vector *vec);

// clears all elements from the vector without shrinking capacity
void vector_clear(Vector *vec);

#endif
