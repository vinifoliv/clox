#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

/**
 * Resets the virtual machine's stack pointer to the beginning of the stack.
 * This effectively empties the stack, preparing it for new data.
 */
static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {

}

/**
 * Pushes a value onto the stack.
 *
 * @param value the value to push
 *
 * This function increments the stack pointer after pushing the value.
 */
void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

/**
 * Pops a value from the stack.
 *
 * @return the popped value
 *
 * This function decrements the stack pointer after popping the value.
 */
Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

/**
 * Executes the bytecode instructions of the current chunk by continuously
 * reading and interpreting each instruction pointed to by the VM's instruction
 * pointer. Uses macros to read the next byte instruction and retrieve constant
 * values. The execution continues indefinitely until an OP_RETURN instruction is
 * encountered, at which point the function returns with an INTERPRET_OK result.
 * In debug mode, disassembles and prints each instruction for tracing purposes.
 *
 * @return INTERPRET_OK upon successful execution of bytecode instructions.
 */
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++) // Gets next instruction and updates IP to the one after it
#define READ_CONSTANT()   \
    (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)     \
    do {                  \
        double b = pop(); \
        double a = pop(); \
        push(a op b);     \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            case OP_NEGATE:   {
                Value* value = vm.stackTop - 1;
                *value = -(*value);
                break;
            }
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

/**
 * Interprets a chunk of bytecode by setting the VM's current chunk
 * and instruction pointer, then executing the bytecode instructions.
 *
 * @param chunk the chunk of bytecode to interpret
 * @return the result of the interpretation process
 */

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}