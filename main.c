#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/**
 * Enters an interactive REPL mode where the user is prompted to enter code
 * which is then interpreted.
 */
static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

/**
 * Reads the contents of a file and returns a null-terminated string.
 *
 * @param path the path to the file to read
 *
 * @return a null-terminated string containing the contents of the file
 *
 * The returned string is allocated using malloc and the caller is responsible
 * for freeing it. If the file cannot be opened, a message is printed to stderr
 * and the program exits with status 74. If there is not enough memory to read
 * the file, a message is printed to stderr and the program exits with status
 * 74. If the file could not be read (e.g. it is not a regular file), a message
 * is printed to stderr.
 */
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

/**
 * Runs a file of source code.
 *
 * @param path the path to the file to run
 *
 * Reads the file, interprets it, and exits with an appropriate status code based
 * on the result of the interpretation.
 */
static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}