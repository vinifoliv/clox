#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

/**
 * Initializes the scanner with a source string.
 *
 * @param source the source string to tokenize
 */
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

/**
 * Checks if a character is a valid identifier character, i.e. a letter (a-z or
 * A-Z) or an underscore.
 *
 * @param c the character to check
 * @return true if the character is a valid identifier character, false
 *         otherwise
 */
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

/**
 * Determines if a character is a numeric digit (0-9).
 *
 * @param c the character to check
 * @return true if the character is a digit, false otherwise
 */
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * Checks whether the scanner is at the end of the source string.
 *
 * @return true if the scanner is at the end of the source string, false otherwise
 */
static bool isAtEnd() {
    return *scanner.current == '\0';
}

/**
 * Advances the scanner to the next character and returns the current character.
 *
 * @return the current character
 */
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

/**
 * Returns the current character without advancing the scanner.
 *
 * @return the current character
 */
static char peek() {
    return *scanner.current;
}

/**
 * Returns the next character in the source string without advancing the scanner.
 *
 * @return the next character or '\0' if at the end of the source string
 */
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

/**
 * Checks if the current character matches the expected character.
 *
 * If the current character is the expected character, moves the scanner to the
 * next character and returns true. Otherwise, stays at the current character and
 * returns false.
 *
 * @param expected the expected character
 * @return true if the characters match, false otherwise
 */
static bool match(char expected) {
    if (isAtEnd()) return false;
    // since advance() move current to the next character, *scanner.current = next character
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

/**
 * Creates a token with the given type and the text from the current position to
 * the start of the scanner.
 *
 * @param type the type of the token
 * @return a newly created Token
 */
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/**
 * Creates an error token with the specified message.
 *
 * @param message the error message to be associated with the token
 * @return a Token with type TOKEN_ERROR, containing the provided message, its length, and the current line number from the scanner
 */
static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

/**
 * Skips all whitespace characters in the source string until a non-whitespace
 * character is found, which is then left as the current character of the
 * scanner.
 *
 * Whitespace characters include spaces, tabs, carriage returns, and line feeds.
 * Line comments are also skipped.
 */
static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/**
 * Checks if the current identifier is a keyword by comparing it with the
 * given string. If the identifier matches the keyword, the function returns
 * the associated TokenType. Otherwise, it returns TOKEN_IDENTIFIER.
 *
 * @param start the starting index of the identifier
 * @param length the length of the keyword
 * @param rest the characters of the keyword
 * @param type the TokenType associated with the keyword
 * @return the TokenType of the keyword, or TOKEN_IDENTIFIER if the identifier
 *         does not match the keyword
 */
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
            return type;
        }
    
    return TOKEN_IDENTIFIER;
}

/**
 * Determines the TokenType of a given identifier by checking if it matches
 * any known keyword. If the identifier is a keyword, the corresponding 
 * TokenType is returned. Otherwise, TOKEN_IDENTIFIER is returned.
 *
 * The function inspects the first character of the identifier and checks
 * subsequent characters to identify keywords such as "and", "class", "else",
 * etc.
 *
 * @return the TokenType corresponding to the keyword if it matches, or
 *         TOKEN_IDENTIFIER if it does not.
 */
static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

/**
 * Scans an identifier token.
 *
 * The scanner is positioned at the beginning of an identifier, and this function
 * advances the scanner over the entire sequence of valid identifier characters.
 * It returns a Token with type TOKEN_IDENTIFIER and the identifier's text.
 *
 * An identifier consists of letters, digits, or underscores, but must start
 * with a letter or underscore.
 *
 * @return a Token representing the scanned identifier
 */
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

/**
 * Scans a number token.
 *
 * The scanner is positioned at the beginning of the number, and the method
 * will advance the scanner to the end of the number.
 *
 * A number is either an integer or a floating-point number. If the scanner
 * encounters a decimal point, it will continue to advance the scanner until it
 * encounters a non-digit character.
 *
 * @return a Token with type TOKEN_NUMBER, containing the number value, its
 *         length, and the current line number from the scanner
 */
static Token number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

/**
 * Scans a string token.
 *
 * The scanner is positioned at the beginning of the string, and the closing
 * quote is consumed by this function. If the string is not closed, an error
 * token is returned.
 *
 * @return a Token with type TOKEN_STRING, containing the string value,
 *         its length, and the line number at the beginning of the string
 */
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (!isAtEnd()) return errorToken("Unterminated string.");

    // The closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

/**
 * Scans a token from the source string and returns the corresponding Token.
 *
 * This method will skip any whitespace characters and then determine the
 * TokenType of the current character. If the character is a letter, it will
 * scan an identifier. If the character is a digit, it will scan a number.
 * Otherwise, it will check if the character matches any single character
 * tokens, and if not, it will check if the character matches any two character
 * tokens.
 *
 * If none of the above match, an error token is returned.
 *
 * @return a Token representing the scanned token
 */
Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        // Single character tokens
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);

        // One or two tokens
        case '!': return makeToken(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG
        );
        case '=': return makeToken(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL
        );
        case '<': return makeToken(
            match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS
        );
        case '>': return makeToken(
            match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER
        );

        // Literals
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}