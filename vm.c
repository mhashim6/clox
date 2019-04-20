#include <stdarg.h>
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

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code;
  fprintf(stderr, "[line %d] in script\n", getLine(vm.chunk, instruction));

  resetStack();
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

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define LONG_CONSTANT_INDEX() \
  ((READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE())
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[LONG_CONSTANT_INDEX()])

#define BINARY_OP(valueType, op)                      \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers.");      \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
                                                      \
    double b = AS_NUMBER(pop());                      \
    double a = AS_NUMBER(pop());                      \
    push(valueType(a op b));                          \
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
        /******************************/

      case OP_NIL:
        push(NIL_VAL);
        break;
      case OP_TRUE:
        push(BOOL_VAL(true));
        break;
      case OP_FALSE:
        push(BOOL_VAL(false));
        break;

        /******************************/

      case OP_ADD: {
        BINARY_OP(NUMBER_VAL, +);
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(NUMBER_VAL, -);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(NUMBER_VAL, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        break;
      }
        /******************************/

      case OP_NEGATE: {
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(NUMBER_VAL(-AS_NUMBER(pop())));
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
  Chunk chunk;
  initChunk(&chunk);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  freeChunk(&chunk);
  return result;
}
