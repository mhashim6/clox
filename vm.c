#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

VM vm;

static void resetStack() {
  Stack* stack = &vm.stack;
  stack->capacity = 0;
  stack->count = 0;
  stack->elements = NULL;
  stack->top = stack->elements;
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

void initVM() {
  initTable(&vm.strings);
  resetStack();
  vm.objects = NULL;
}

void freeVM() {
  freeTable(&vm.strings);
  freeObjects();
}

void push(Value value) {
  Stack* stack = &vm.stack;

  if (stack->count + 1 > stack->capacity) {
    int stackTopOffset = stack->top - stack->elements;
    stack->capacity = GROW_CAPACITY(stack->capacity, STACK_MAX);
    stack->elements =
        GROW_ARRAY(stack->elements, Value, stack->count, stack->capacity);
    stack->top = stack->elements + stackTopOffset;
  }

  *stack->top = value;
  stack->top++;
  stack->count++;
}
Value pop() {
  Stack* stack = &vm.stack;

  stack->top--;
  stack->count--;
  stack->capacity = GROW_CAPACITY(stack->capacity, STACK_MAX);
  return *stack->top;
}

static Value peek(int distance) { return vm.stack.top[-1 - distance]; }

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) &&
                           !AS_BOOL(value));  // TODO: this yields: !nil = false
}

static void concatenate() {
  ObjString* b = LOX_STRING(pop());
  ObjString* a = LOX_STRING(pop());

  int length = a->length + b->length;

  char* chars = ALLOCATE(char, length + 1);
  printf("I'm called\n");
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(LOX_OBJ(result));
}

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
    for (Value* slot = vm.stack.elements; slot < vm.stack.top; slot++) {
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
        push(LOX_NIL);
        break;
      case OP_TRUE:
        push(LOX_BOOL(true));
        break;
      case OP_FALSE:
        push(LOX_BOOL(false));
        break;

        /******************************/

      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(LOX_BOOL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER:
        BINARY_OP(LOX_BOOL, >);
        break;
      case OP_LESS:
        BINARY_OP(LOX_BOOL, <);
        break;

        /******************************/

      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(LOX_NUMBER(a + b));
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(LOX_NUMBER, -);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(LOX_NUMBER, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(LOX_NUMBER, /);
        break;
      }
        /******************************/
      case OP_NOT: {
        push(LOX_BOOL(!isFalsey(pop())));
        break;
      }
      case OP_NEGATE: {
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(LOX_NUMBER(-AS_NUMBER(pop())));
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
