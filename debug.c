#include <stdio.h>

#include "debug.h"
#include "value.h"

/**
 * Disassembles a chunk of bytecode instructions and prints them.
 *
 * @param chunk the chunk of bytecode to disassemble
 * @param name the name of the chunk, used for display purposes
 */

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

/**
 * Prints a bytecode instruction that has a single constant argument.
 *
 * @param name the name of the instruction
 * @param chunk the chunk of bytecode that contains the instruction
 * @param offset the offset of the instruction in the chunk
 *
 * @return the offset of the instruction after the one that was disassembled
 */
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

/**
 * Prints out a simple bytecode instruction with no additional arguments.
 *
 * @param name the name of the instruction
 * @param offset the offset of the instruction in the chunk
 *
 * @return the offset of the instruction after the one that was disassembled
 */
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/**
 * Disassembles a single bytecode instruction and prints it.
 *
 * @param chunk the chunk of bytecode that contains the instruction
 * @param offset the offset of the instruction in the chunk
 *
 * @return the offset of the instruction after the one that was disassembled
 */
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset); // 04 -> outputs a 4 digit output (filled with 0s if necessary)

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}