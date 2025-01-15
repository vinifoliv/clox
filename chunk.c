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
}

/**
 * Frees a chunk's memory.
 *
 * @param chunk the chunk to free
 */
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    initChunk(chunk);
}

/**
 * Adds a single byte to the end of a chunk's code.
 *
 * @param chunk the chunk to write to
 * @param byte the byte to write
 */
void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}