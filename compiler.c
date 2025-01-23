#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

/**
 * Reports an error at the given token with the given message.
 *
 * The parser will only report one error at a time, so if the parser is in
 * panic mode, this function does nothing. Otherwise, it will print the error
 * message to stderr and set the parser's panic mode to true.
 *
 * @param token the token at which the error occurred
 * @param message the error message to report
 */
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

/**
 * Reports an error at the position immediately preceding the current position
 * in the parser.
 *
 * @param message the error message to report
 */
static void error(const char* message) {
    errorAt(&parser.previous, message);
}

/**
 * Reports an error at the current position in the parser.
 *
 * @param message the error message to report
 */
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * Advances the parser to the next token and discards any encountered errors.
 *
 * If the current token is an error token, this method will continuously scan
 * the source and report any encountered errors until a non-error token is
 * found. The parser is then positioned at the first non-error token after the
 * last encountered error. If no errors are encountered, the parser is simply
 * positioned at the next token.
 */
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

/**
 * Emits a single byte of bytecode at the current position in the chunk.
 *
 * This function appends the given byte to the current chunk's bytecode and
 * records the line number of the given byte as the line number of the bytecode
 * instruction.
 *
 * @param byte the byte to emit
 */
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * Emits two bytes of bytecode at the current position in the chunk.
 *
 * This function is a convenience wrapper around emitByte that emits two bytes
 * of bytecode at once. This is useful for emitting bytecode instructions that
 * have an argument, such as OP_CONSTANT or OP_JUMP.
 *
 * @param byte1 the first byte of the instruction
 * @param byte2 the second byte of the instruction
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

/**
 * Adds a constant value to the current chunk and returns its index.
 *
 * This function appends the provided constant value to the constants array
 * of the current chunk. If the number of constants exceeds the maximum 
 * allowable index for a single byte, an error is reported.
 *
 * @param value the constant value to add
 * @return the index of the added constant in the constants array, or 0 if 
 *         an error occurred due to exceeding the maximum allowed constants
 */
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

/**
 * Emits an OP_CONSTANT bytecode instruction with the given constant value.
 *
 * This function adds a value to the current chunk's constants array and
 * emits an OP_CONSTANT bytecode instruction with the index of the added
 * value. The instruction is emitted at the current position in the chunk.
 *
 * @param value the constant value to emit
 */
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * Completes the current compilation process and emits a return instruction.
 *
 * This function appends an OP_RETURN bytecode instruction to the current
 * chunk, signaling the end of the compiled code. It is typically called
 * after all expressions or statements have been compiled.
 */
static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        default: return; // Unreachable
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable
    }
}

/**
 * Parses a grouped expression enclosed in parentheses.
 *
 * This function begins by parsing an expression and then expects a closing
 * right parenthesis token. If the closing parenthesis is not found, an error
 * message is reported. It assumes the current token is the opening parenthesis.
 */
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * Compiles a number literal.
 *
 * This function assumes that the current token is a number literal. It emits
 * the number as a constant bytecode instruction. The value of the number is
 * converted to a double using strtod.
 */
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/**
 * Parses a unary expression.
 *
 * This function processes a unary operator by first determining the type of the
 * operator from the parser's previous token. It then parses the operand of the
 * unary operator. Depending on the operator type, it emits the corresponding
 * bytecode instruction: either OP_NOT for a logical negation or OP_NEGATE for
 * arithmetic negation.
 */
static void unary() {
    TokenType operatorType  = parser.previous.type;

    // Gets the operand
    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL ,  PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE}
};

/**
 * Parses an expression with a given precedence.
 *
 * This method will advance the parser to the next token and then parse the
 * expression using the prefix rule associated with the current token. If the
 * current token does not have a prefix rule (i.e. it is not an expression), an
 * error message is reported. Otherwise, the prefix rule is called to parse the
 * expression.
 *
 * After the expression is parsed, the method will then parse any trailing
 * infix expressions until the precedence of the trailing expressions is
 * less than the given precedence. This is done by repeatedly advancing to the
 * next token and calling the infix rule associated with the current token.
 *
 * @param precedence the precedence of the expressions to parse
 */
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

/**
 * Retrieves the ParseRule associated with the given TokenType.
 *
 * @param type the TokenType whose associated ParseRule is to be retrieved
 * @return a pointer to the ParseRule associated with the given TokenType
 */
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}


/**
 * Parses an expression with the lowest precedence (assignment).
 *
 * This method will parse an expression with the lowest precedence by calling
 * parsePrecedence with the PREC_ASSIGNMENT parameter. This will parse an
 * expression with any precedence since assignment has the lowest precedence.
 */
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * Compiles the given source code into bytecode and stores it in the provided chunk.
 *
 * This function initializes the scanner with the source code and sets up the
 * compiling environment. It processes the source code by advancing through
 * end of the source is reached, at which point an OP_RETURN instruction is emitted.
 * Any errors encountered during compilation set the parser's hadError flag to true.
 *
 * @param source the source code to compile
 * @param chunk the chunk where the compiled bytecode will be stored
 * @return true if the compilation was successful without errors, false otherwise
 */
bool compile(const char* source, Chunk* chunk) { 
    initScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}