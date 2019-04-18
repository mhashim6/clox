#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() { vm.stackTop = vm.stack; }

void initVM() { resetStack(); }

void freeVM() {}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}
Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define LONG_CONSTANT_INDEX() \
  ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE())
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[LONG_CONSTANT_INDEX()])

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CONSTANT_LONG: {
        Value constant = READ_LONG_CONSTANT();
        push(constant);
        break;
      }
      case OP_RETURN: {
        printValue(pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef LONG_CONSTANT_INDEX
#undef READ_LONG_CONSTANT
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = chunk->code;
  return run();
}
