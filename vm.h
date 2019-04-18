#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  /*instruction pointer, aka PC*/
  uint8_t *ip;
  Value *stack;
  Value *stackTop;
  int stackCount;
  int stackCapacity;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();

InterpretResult interpret(Chunk *chunk);

void push(Value value);
Value pop();

#endif