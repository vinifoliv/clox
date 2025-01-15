#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/**
 * Initializes a chunk with no code.
 *
 * @param chunk the chunk to initialize
 */
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

/**
 * Frees a chunk's memory.
 *
 * @param chunk the chunk to free
 */
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/**
 * Adds a single byte to the end of a chunk's code.
 *
 * @param chunk the chunk to write to
 * @param byte the byte to write
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

/**
 * Adds a constant value to the chunk's constants array.
 *
 * @param chunk the chunk to which the constant is added
 * @param value the constant value to add
 * @return the index of the added constant in the constants array
 */
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}