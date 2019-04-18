#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char const *argv[]) {
  initVM();

  Chunk chunk;
  initChunk(&chunk);
  writeConstant(&chunk, 1.2, 1);
  writeConstant(&chunk, 3.4, 1);
  writeChunk(&chunk, OP_ADD, 1);

  writeConstant(&chunk, 5.6, 1);
  writeChunk(&chunk, OP_DIVIDE, 1);

  writeChunk(&chunk, OP_NEGATE, 1);
  writeChunk(&chunk, OP_RETURN, 1);
  interpret(&chunk);
  freeVM();
  freeChunk(&chunk);
  return 0;
}
