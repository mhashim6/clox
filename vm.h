#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
  Value *elements;
  Value *top;
  int count;
  int capacity;
} Stack;

typedef struct {
  Chunk *chunk;
  /*instruction pointer, aka PC*/
  uint8_t *ip;
  Stack stack;
  Obj *objects;
  Table strings; //for string interning.
  Table globals;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();

InterpretResult interpret(const char *source);
extern VM vm;

void push(Value value);
Value pop();

#endif