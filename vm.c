#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
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

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
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
 * Returns the value at a specified distance from the top of the stack
 * without removing it. The distance is measured from the top of the stack,
 * where a distance of 0 refers to the topmost element.
 *
 * @param distance the distance from the top of the stack
 * @return the value at the specified distance
 */

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
#define READ_CONSTANT()                                   \
    (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op)                                     \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
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
            case OP_NIL:      push(NIL_VAL); break;
            case OP_TRUE:     push(BOOL_VAL(true)); break;
            case OP_FALSE:    push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
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
 * Interprets the given source code by compiling it and returning the result.
 *
 * @param source the source code to interpret
 * @return INTERPRET_OK upon successful compilation
 */
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}