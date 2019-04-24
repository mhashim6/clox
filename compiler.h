#ifndef clox_compiler_h
#define clox_compiler_h

#include "parser.h"
#include "vm.h"

#define GLOBAL_SCOPE 0
#define UNINITIALIZED -1

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} CompilerState;

bool compile(const char* source, Chunk* chunk);

#endif