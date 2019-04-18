#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

typedef struct
{
    Chunk *chunk;
    /*instruction pointer, aka PC*/
    uint8_t *ip;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk *chunk);

#endif