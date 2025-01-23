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
 * Prints a value to the standard output.
 *
 * @param value the value to print
 */
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    }
}

/**
 * Checks if two values are equal. The values are equal if they have the same
 * type and their values are equal.
 *
 * @param a the first value to compare
 * @param b the second value to compare
 * @return true if the values are equal, false otherwise
 */
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        default:         return false; // Unreachable
    }
}