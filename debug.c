#include <stdio.h>

#include "debug.h"

void disassembleChunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
  uint8_t const_index = chunk->code[offset + 1];
  printf("%-16s %4d '", name, const_index);
  printValue(chunk->constants.values[const_index]);
  printf("'\n");
  return offset + 2;
}

static int longConstantInstruction(const char *name, Chunk *chunk, int offset) {
#define LONG_CONSTANT_INDEX(code, offset) \
  ((code[offset + 1] << 16) | (code[offset + 2] << 8) | (code[offset + 3]))

  uint32_t const_index = LONG_CONSTANT_INDEX(chunk->code, offset);
  printf("%-16s %4d '", name, const_index);

  printValue(chunk->constants.values[const_index]);
  printf("'\n");
  return offset + 4;
#undef LONG_CONSTANT_INDEX
}

static int simpleInstruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  int line = getLine(chunk, offset);
  int prev_instr_line = getLine(chunk, offset - 1);
  if (line == prev_instr_line) {
    printf("   | ");
  } else {
    printf("%4d ", line);
  }
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
      return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
