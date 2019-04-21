#ifndef clox_memory_h
#define clox_memory_h

#include "memory.h"
#include "object.h"
#include "vm.h"

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity, min) ((capacity) < (min) ? (min) : (capacity)*2)

#define GROW_ARRAY(previous, type, oldCount, count)       \
  (type *)reallocate(previous, sizeof(type) * (oldCount), \
                     sizeof(type) * (count))

#define FREE_ARRAY(type, pointer, oldCount) \
  (type *)reallocate((pointer), sizeof(type) * (oldCount), 0)

void *reallocate(void *previous, size_t oldSize, size_t newSize);

void freeObjects();

#define ALLOCATE(type, count) \
  (type *)reallocate(NULL, 0, sizeof(type) * (count))

#endif