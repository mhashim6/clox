#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, char const *argv[])
{
    Chunk chunk;
    initChunk(&chunk);
    int const_index = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, const_index);
    writeChunk(&chunk, OP_RETURN);
    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);
    return 0;
}
