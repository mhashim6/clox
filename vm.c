#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"

VM vm;

static void resetStack() {
  vm.stackCapacity = 0;
  vm.stackCount = 0;
  vm.stack = NULL;
  vm.stackTop = vm.stack;
}

void initVM() { resetStack(); }

void freeVM() {}

void push(Value value) {
  if (vm.stackCount + 1 > vm.stackCapacity) {
    int stackTopOffset = vm.stackTop - vm.stack;
    vm.stackCapacity = GROW_CAPACITY(vm.stackCapacity, STACK_MAX);
    vm.stack = GROW_ARRAY(vm.stack, Value, vm.stackCount, vm.stackCapacity);
    vm.stackTop = vm.stack + stackTopOffset;
  }
  
  *vm.stackTop = value;
  vm.stackTop++;
  vm.stackCount++;
}
Value pop() {
  vm.stackTop--;
  vm.stackCount--;
  return *vm.stackTop;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define LONG_CONSTANT_INDEX() \
  ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE())
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[LONG_CONSTANT_INDEX()])

#define BINARY_OP(op) \
  do {                \
    double b = pop(); \
    double a = pop(); \
    push(a op b);     \
  } while (false)

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
      case OP_ADD: {
        BINARY_OP(+);
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(-);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(*);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(/);
        break;
      }
      case OP_NEGATE: {
        push(-pop());
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
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
  compile(source);
  return INTERPRET_OK;
}
