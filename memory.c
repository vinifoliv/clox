#include <stdlib.h>

#include "memory.h"


/**
 * Reallocates a block of memory to be of a different size.
 *
 * @param pointer the block of memory to reallocate
 * @param oldSize the old size of the block of memory
 * @param newSize the new size of the block of memory
 *
 * If newSize is 0, the memory is freed and NULL is returned. If the
 * reallocation fails, the program exits with status 1.
 *
 * @return the reallocated block of memory
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}