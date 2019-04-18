#include <stdio.h>

#include "debug.h"

void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t const_index = chunk->code[offset + 1];
    printf("%-16s %4d '", name, const_index);
    printValue(chunk->constants.values[const_index]);
    printf("'\n");
    return offset + 2;
}

static int longConstantInstruction(const char *name, Chunk *chunk, int offset)
{
#define LONG_CONSTANT_INDEX(code, offset) \
    ((code[offset + 1] << 16) | (code[offset + 2] << 8) | (code[offset + 3]))

    uint32_t const_index = LONG_CONSTANT_INDEX(chunk->code, offset);
    printf("%-16s %4d '", name, const_index);

    printValue(chunk->constants.values[const_index]);
    printf("'\n");
    return offset + 4;
#undef LONG_CONSTANT_INDEX
}

static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    int line = getLine(chunk, offset);
    int prev_instr_line = getLine(chunk, offset - 1);
    if (line == prev_instr_line)
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", line);
    }
    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_LONG:
        return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
