#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(chunk->lines, int, oldCapacity, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->lines[line]++;

  chunk->count++;
}

int addConstant(Chunk *chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

void writeConstant(Chunk *chunk, Value value, int line) {
  int const_index = addConstant(chunk, value);
  if (const_index <= MAX_BYTE_ADDRESS) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, const_index, line);
  } else {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, const_index >> (2 * 8), line);     // 1st byte
    writeChunk(chunk, (const_index >> 8) & 0xFF, line);  // 2nd byte
    writeChunk(chunk, const_index & 0xFF, line);         // 1st byte
  }
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

int getLine(Chunk *chunk, int offset) {
  /*
    for each line slot
      if current offset == offset
          it's our line.
      else
          increment current offset by slot's data
  */

  if (offset <= 0)  // handle initial instruction.
    return offset + 1;

  int cur_offset = 0;
  int line = 1; /*lines start from 1*/

  for (int i = line; i <= chunk->count; i++) {
    if (cur_offset == offset) {
      line = i;
      break;
    }
    cur_offset += chunk->lines[i];
  }
  return line;
}