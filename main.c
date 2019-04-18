#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char const *argv[]) {
  initVM();

  Chunk chunk;
  initChunk(&chunk);
  writeConstant(&chunk, 1.2, 1);
  writeChunk(&chunk, OP_RETURN, 2);
  interpret(&chunk);
  freeVM();
  freeChunk(&chunk);
  return 0;
}
