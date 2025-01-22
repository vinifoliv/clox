#include <stdio.h>

#include "memory.h"
#include "value.h"

/**
 * Initializes the array of values for a chunk of bytecode.
 * 
 * @param array the array to be initilialized
 */
void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

/**
 * Adds a value to a chunk's array of values, growing the array as
 * necessary.
 *
 * @param array the ValueArray to add the value to
 * @param value the value to add
 */
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

/**
 * Frees a chunk's array of values memory and reinitializes it.asm
 * 
 * @param array the array of values to be freed
 */
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

/**
 * Prints a Value to the console. The value is printed as a double.
 *
 * @param value the value to print
 */
void printValue(Value value) {
    printf("%g", AS_NUMBER(value));
}