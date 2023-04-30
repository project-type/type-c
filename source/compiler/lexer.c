//
// Created by praisethemoon on 28.04.23.
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>
#include "lexer.h"

/*
 * First we define some utilities for our lexer
 */

/**
 * Checks if the lexer reached its end
 * @param lexerState
 * @return true if lexer is at end, false otherwise
 */
uint8_t isAtEnd(LexerState* lexerState) {
    return lexerState->buffer[lexerState->pos] == '\0';
}

/**
 * Returns current character, even if it is the end
 * @param lexerState
 * @return current character
 */
char getCurrentChar(LexerState* lexerState) {
    return lexerState->buffer[lexerState->pos];
}

/**
 * Increments lexer's position and updates line/col
 * @param lexer
 */
void incLexer(LexerState* lexer) {
    if(getCurrentChar(lexer) == '\n') {
        lexer->line++;
        lexer->col = 0;
    }
    else {
        lexer->col++;
    }
    lexer->pos++;
}


/**
 * Matches lexer state with a sequence of characters. Either current states matches, or not.
 * Also if the match has been successful, it keeps the lexer's position. Otherwise, it
 * reverts the lexer to its original state
 * @param lexerState
 * @param pattern
 * @return true of matches, 0 otherwise
 */
uint8_t match(LexerState* lexerState, const char* pattern) {
    uint32_t i = 0;
    uint32_t len = strlen(pattern);

    LexerState oldState = *lexerState;

    for(; (i < len) && (!isAtEnd(lexerState)); i++, incLexer(lexerState)){
        if(getCurrentChar(lexerState) != pattern[i]) {
            // invalid match, put old position back
            lexerState->pos = oldState.pos;
            lexerState->line = oldState.line;
            lexerState->col = oldState.col;
            return 0;
        }
    }

    // we keep lexerState->pos to its new position
    return 1;
}

/**
 * Matches lexer state with keywords. Either current states matches, or not.
 * Also if the match has been successful, it keeps the lexer's position. Otherwise, it
 * reverts the lexer to its original state. It only returns true if an escape sequence or new line
 * follows the pattern
 * @param lexerState
 * @param pattern
 * @return true of matches, 0 otherwise
 */
uint8_t match_keyword(LexerState* lexerState, const char* pattern) {
    uint32_t i = 0;
    uint32_t len = strlen(pattern);

    LexerState oldState = *lexerState;

    for(; (i < len) && (!isAtEnd(lexerState)); i++, incLexer(lexerState)){
        if(getCurrentChar(lexerState) != pattern[i]) {
            // invalid match, put old position back
            lexerState->pos = oldState.pos;
            lexerState->line = oldState.line;
            lexerState->col = oldState.col;
            return 0;
        }
    }

    if(isalpha(getCurrentChar(lexerState))){
        return 0;
    }


    // we keep lexerState->pos to its new position
    return 1;
}

/**
 * Returns next character
 * @param lexerState
 * @return next character (or \0 on end)
 */
char getNextChar(LexerState* lexerState) {
    if (lexerState->pos > lexerState->len){
        return '\0';
    }
    return lexerState->buffer[lexerState->pos+1];
}

/**
 * Skips spaces/comments/new lines
 * @param lexerState
 */
void skipSpaces(LexerState* lexerState) {
    uint8_t seenSkippable = 0;

    if(getCurrentChar(lexerState) == '\0') {
        return;
    }

    while (getCurrentChar(lexerState) == ' ' || getCurrentChar(lexerState) == '\n' ) {
        seenSkippable = 1;
        incLexer(lexerState);
    }

    if(match(lexerState, "//")){
        seenSkippable = 1;
        uint8_t at_end = match(lexerState, "\n") || isAtEnd(lexerState);
        while(!at_end) {
            incLexer(lexerState);
            at_end = match(lexerState, "\n") || isAtEnd(lexerState);
        }
    }

    if(match(lexerState, "/*")){
        seenSkippable = 1;
        incLexer(lexerState);
        uint8_t at_end = match(lexerState, "*/") || isAtEnd(lexerState);
        while(!at_end) {
            incLexer(lexerState);
            at_end = match(lexerState, "*/") || isAtEnd(lexerState);
        }
    }

    if(seenSkippable) {
        skipSpaces(lexerState);
    }
}

/**
 * Prints lexer's current line, col and pos
 * @param lexerState
 */
void debugLexer(LexerState* lexerState) {
    printf("Lexer\n\tpos: %llu\n\tline: %llu\n\tcol:%llu\n", lexerState->pos, lexerState->line, lexerState->col);
}

/*
 * Main lexer code:
 */

LexerState* lexer_init(const char* filename, const char* buffer, uint64_t len) {
    LexerState * lexer = malloc(sizeof(LexerState));
    lexer->buffer = strdup(buffer);
    lexer->filename = strdup(filename);
    lexer->pos = 0;
    lexer->col = 0;
    lexer->len = strlen(buffer);
    lexer->line = 1;
    lexer->col = 0;

    return lexer;
}

Lexeme lexer_next(LexerState* lexerState) {

}

Lexeme lexer_peek(LexerState* lexerState) {

}

void lexer_free(LexerState* lexerState) {

}

Lexeme makeLexem(TokenType type, const char* value, LexerState* lexerState) {
    Lexeme lexeme = {type, value, lexerState->line, lexerState->col};
    return lexeme;
}

Lexeme makeLexemLineCol(TokenType type, const char* value, uint32_t line, uint32_t col) {
    Lexeme lexeme = {type, value, line, col};
    return lexeme;
}

Lexeme lexIdOrKeyword(LexerState* lexerState) {
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;

    // check for keyword:
    if(match_keyword(lexerState, "any")) return makeLexemLineCol(TOK_ANY, NULL, line, col);
    if(match_keyword(lexerState, "async")) return makeLexemLineCol(TOK_ASYNC, NULL, line, col);
    if(match_keyword(lexerState, "await")) return makeLexemLineCol(TOK_AWAIT, NULL, line, col);
    if(match_keyword(lexerState, "as")) return makeLexemLineCol(TOK_TYPE_CONVERSION, NULL, line, col);
    if(match_keyword(lexerState, "break")) return makeLexemLineCol(TOK_BREAK, NULL, line, col);
    if(match_keyword(lexerState, "case")) return makeLexemLineCol(TOK_CASE, NULL, line, col);
    if(match_keyword(lexerState, "class")) return makeLexemLineCol(TOK_CLASS, NULL, line, col);
    if(match_keyword(lexerState, "continue")) return makeLexemLineCol(TOK_CONTINUE, NULL, line, col);
    if(match_keyword(lexerState, "mut")) return makeLexemLineCol(TOK_MUT, NULL, line, col);
    if(match_keyword(lexerState, "variant")) return makeLexemLineCol(TOK_VARIANT, NULL, line, col);
    if(match_keyword(lexerState, "delete")) return makeLexemLineCol(TOK_DELETE, NULL, line, col);
    if(match_keyword(lexerState, "do")) return makeLexemLineCol(TOK_DO, NULL, line, col);
    if(match_keyword(lexerState, "else")) return makeLexemLineCol(TOK_ELSE, NULL, line, col);
    if(match_keyword(lexerState, "enum")) return makeLexemLineCol(TOK_ENUM, NULL, line, col);
    if(match_keyword(lexerState, "extern")) return makeLexemLineCol(TOK_EXTERN, NULL, line, col);
    if(match_keyword(lexerState, "false")) return makeLexemLineCol(TOK_FALSE, NULL, line, col);
    if(match_keyword(lexerState, "from")) return makeLexemLineCol(TOK_FROM, NULL, line, col);
    if(match_keyword(lexerState, "for")) return makeLexemLineCol(TOK_FOR, NULL, line, col);
    if(match_keyword(lexerState, "foreach")) return makeLexemLineCol(TOK_FOREACH, NULL, line, col);
    if(match_keyword(lexerState, "fn")) return makeLexemLineCol(TOK_FUNCTION, NULL, line, col);
    if(match_keyword(lexerState, "if")) return makeLexemLineCol(TOK_IF, NULL, line, col);
    if(match_keyword(lexerState, "import")) return makeLexemLineCol(TOK_IMPORT, NULL, line, col);
    if(match_keyword(lexerState, "in")) return makeLexemLineCol(TOK_IN, NULL, line, col);
    if(match_keyword(lexerState, "is")) return makeLexemLineCol(TOK_IS, NULL, line, col);
    if(match_keyword(lexerState, "interface")) return makeLexemLineCol(TOK_INTERFACE, NULL, line, col);
    if(match_keyword(lexerState, "i8")) return makeLexemLineCol(TOK_I8, NULL, line, col);
    if(match_keyword(lexerState, "u8")) return makeLexemLineCol(TOK_U8, NULL, line, col);
    if(match_keyword(lexerState, "i16")) return makeLexemLineCol(TOK_I16, NULL, line, col);
    if(match_keyword(lexerState, "u16")) return makeLexemLineCol(TOK_U16, NULL, line, col);
    if(match_keyword(lexerState, "i32")) return makeLexemLineCol(TOK_I32, NULL, line, col);
    if(match_keyword(lexerState, "u32")) return makeLexemLineCol(TOK_U32, NULL, line, col);
    if(match_keyword(lexerState, "i64")) return makeLexemLineCol(TOK_I64, NULL, line, col);
    if(match_keyword(lexerState, "u64")) return makeLexemLineCol(TOK_U64, NULL, line, col);
    if(match_keyword(lexerState, "f32")) return makeLexemLineCol(TOK_F32, NULL, line, col);
    if(match_keyword(lexerState, "f64")) return makeLexemLineCol(TOK_F64, NULL, line, col);
    if(match_keyword(lexerState, "bool")) return makeLexemLineCol(TOK_BOOLEAN, NULL, line, col);
    if(match_keyword(lexerState, "string")) return makeLexemLineCol(TOK_STRING, NULL, line, col);
    if(match_keyword(lexerState, "char")) return makeLexemLineCol(TOK_CHAR, NULL, line, col);
    if(match_keyword(lexerState, "let")) return makeLexemLineCol(TOK_LET, NULL, line, col);
    if(match_keyword(lexerState, "new")) return makeLexemLineCol(TOK_NEW, NULL, line, col);
    if(match_keyword(lexerState, "null")) return makeLexemLineCol(TOK_NULL, NULL, line, col);
    if(match_keyword(lexerState, "unsafe")) return makeLexemLineCol(TOK_UNSAFE, NULL, line, col);
    if(match_keyword(lexerState, "ptr")) return makeLexemLineCol(TOK_PTR, NULL, line, col);
    if(match_keyword(lexerState, "return")) return makeLexemLineCol(TOK_RETURN, NULL, line, col);
    if(match_keyword(lexerState, "sizeof")) return makeLexemLineCol(TOK_SIZEOF, NULL, line, col);
    if(match_keyword(lexerState, "super")) return makeLexemLineCol(TOK_SUPER, NULL, line, col);
    if(match_keyword(lexerState, "self")) return makeLexemLineCol(TOK_SELF, NULL, line, col);
    if(match_keyword(lexerState, "static")) return makeLexemLineCol(TOK_STATIC, NULL, line, col);
    if(match_keyword(lexerState, "struct")) return makeLexemLineCol(TOK_STRUCT, NULL, line, col);
    if(match_keyword(lexerState, "match")) return makeLexemLineCol(TOK_MATCH, NULL, line, col);
    if(match_keyword(lexerState, "true")) return makeLexemLineCol(TOK_TRUE, NULL, line, col);
    if(match_keyword(lexerState, "type")) return makeLexemLineCol(TOK_TYPE, NULL, line, col);
    if(match_keyword(lexerState, "void")) return makeLexemLineCol(TOK_VOID, NULL, line, col);
    if(match(lexerState, "while")) return makeLexemLineCol(TOK_WHILE, NULL, line, col);

    while(isalnum(getCurrentChar(lexerState)) || getCurrentChar(lexerState)=='_') {
        incLexer(lexerState);
    }

    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_IDENTIFIER, str_val, line, col);
    return lexeme;
}

Lexeme lexBinaryValue(LexerState* lexerState){
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    char c = getCurrentChar(lexerState);
    while(c == '0' || c == '1') {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_BINARY_INT, str_val, line, col);
    return lexeme;
}


Lexeme lexHexValue(LexerState* lexerState){
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    char c = getCurrentChar(lexerState);
    while(ishexnumber(c)) {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_HEX_INT, str_val, line, col);
    return lexeme;
}

Lexeme lexOctalValue(LexerState* lexerState){
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    char c = getCurrentChar(lexerState);
    while((c >= '0') && (c <= '7')) {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_OCT_INT, str_val, line, col);
    return lexeme;
}


Lexeme lexString(LexerState* lexerState){
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    // skip first quotes
    incLexer(lexerState);
    char c = getCurrentChar(lexerState);
    while(c != '"') {
        incLexer(lexerState);
        // skip \"
        match(lexerState, "\\\"");
        c = getCurrentChar(lexerState);
    }

    incLexer(lexerState);
    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_STRING_VAL, str_val, line, col);
    return lexeme;
}



Lexeme lexChar(LexerState* lexerState){
    // TODO: assert char len is 1

    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    // skip first quotes
    incLexer(lexerState);
    char c = getCurrentChar(lexerState);
    while(c != '\'') {
        incLexer(lexerState);
        // skip \"
        match(lexerState, "\\\'");
        c = getCurrentChar(lexerState);
    }

    incLexer(lexerState);
    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_CHAR_VAL, str_val, line, col);
    return lexeme;
}


Lexeme lexNumber(LexerState* lexerState){
    uint64_t start = lexerState->pos;
    uint32_t line = lexerState->line;
    uint32_t col = lexerState->col;
    char c = getCurrentChar(lexerState);

    while(isnumber(c)) {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    // if the decimal ends with f or d
    if(c == 'f' || c == 'd'){
        uint64_t len = lexerState->pos - start + 1;
        char* str_val = malloc(len*sizeof(char));
        memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
        str_val[len-1] = '\0';

        Lexeme lexeme = makeLexemLineCol(c=='f'?TOK_FLOAT:TOK_DOUBLE, str_val, line, col);
        incLexer(lexerState);
        return lexeme;
    }

    // if we have no dot
    if(c != '.') {
        uint64_t len = lexerState->pos - start + 1;
        char* str_val = malloc(len*sizeof(char));
        memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
        str_val[len-1] = '\0';

        Lexeme lexeme = makeLexemLineCol(TOK_INT, str_val, line, col);
        return lexeme;
    }

    // we have a dot, keep cooking

    incLexer(lexerState);
    c = getCurrentChar(lexerState);
    while(isnumber(c)) {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    // if the trailing ends with f or d, or no dot is present after trailing
    if(c == 'f' || c == 'd' || c != 'e'){
        uint64_t len = lexerState->pos - start + 1;
        char* str_val = malloc(len*sizeof(char));
        memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
        str_val[len-1] = '\0';
        // d must be present for double, otherwise we presume its float.
        Lexeme lexeme = makeLexemLineCol(c=='d'?TOK_DOUBLE:TOK_FLOAT, str_val, line, col);
        incLexer(lexerState);
        return lexeme;
    }

    // if we are here, means we have the exponent
    uint8_t isNegative = 0;
    if (match(lexerState, "e-")){
        isNegative = 1;
    }
    else{
        if(match(lexerState, "e+")){}
        else if (match(lexerState, "e")) {}
    }
    c = getCurrentChar(lexerState);
    while(isnumber(c)) {
        incLexer(lexerState);
        c = getCurrentChar(lexerState);
    }

    if(c == 'f' || c == 'd'){
        uint64_t len = lexerState->pos - start + 1;
        char* str_val = malloc(len*sizeof(char));
        memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
        str_val[len-1] = '\0';

        Lexeme lexeme = makeLexemLineCol(c=='f'?TOK_FLOAT:TOK_DOUBLE, str_val, line, col);
        incLexer(lexerState);
        return lexeme;
    }

    uint64_t len = lexerState->pos - start + 1;
    char* str_val = malloc(len*sizeof(char));
    memcpy(str_val, lexerState->buffer+start, len*sizeof(char)-1);
    str_val[len-1] = '\0';

    Lexeme lexeme = makeLexemLineCol(TOK_FLOAT, str_val, line, col);
    return lexeme;
}

Lexeme lexer_lexCurrent(LexerState* lex) {
    Lexeme current;
    uint32_t line = lex->line;
    uint32_t col = lex->col;

    skipSpaces(lex);

    const char c = getCurrentChar(lex);

    switch (c) {
        case '+': {
            if(match(lex, "++")) {
                return makeLexemLineCol(TOK_INCREMENT, NULL, line, col);
            }
            if(match(lex, "+=")) {
                return makeLexemLineCol(TOK_PLUS_EQUAL, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_PLUS, NULL, line, col);
        }

        case '-': {
            if(match(lex, "--")) {
                return makeLexemLineCol(TOK_DECREMENT, NULL, line, col);
            }

            if(match(lex, "-=")) {
                return makeLexemLineCol(TOK_MINUS_EQUAL, NULL, line, col);
            }

            if(match(lex, "->")) {
                return makeLexemLineCol(TOK_FN_RETURN_TYPE, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_MINUS, NULL, line, col);
        }

        case '*': {
            if(match(lex, "*=")) {
                return makeLexemLineCol(TOK_STAR_EQUAL, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_STAR, NULL, line, col);
        }

        case '/': {
            if(match(lex, "/=")) {
                return makeLexemLineCol(TOK_DIV_EQUAL, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_DIV, NULL, line, col);
        }

        case '%':
            incLexer(lex);
            return makeLexemLineCol(TOK_PERCENT, NULL, line, col);


        case '&': {
            if(match(lex, "&&")) {
                return makeLexemLineCol(TOK_LOGICAL_AND, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_BITWISE_AND, NULL, line, col);
        }

        case '^':
            incLexer(lex);
            return makeLexemLineCol(TOK_BITWISE_XOR, NULL, line, col);


        case '|': {
            if(match(lex, "||")) {
                return makeLexemLineCol(TOK_LOGICAL_OR, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_BITWISE_OR, NULL, line, col);
        }

        case '!': {
            if(match(lex, "!=")) {
                return makeLexemLineCol(TOK_NOT_EQUAL, NULL, line, col);
            }
            if(match(lex, "!!")) {
                return makeLexemLineCol(TOK_DENULL, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_NOT, NULL, line, col);
        }
        case '~': {
            incLexer(lex);
            return makeLexemLineCol(TOK_BITWISE_NOT, NULL, line, col);
        }

        case '=': {
            if(match(lex, "==")) {
                return makeLexemLineCol(TOK_EQUAL_EQUAL, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_EQUAL, NULL, line, col);
        }
        case '<': {
            if(match(lex, "<=")) {
                return makeLexemLineCol(TOK_LESS_EQUAL, NULL, line, col);
            }

            if(match(lex, "<<")) {
                return makeLexemLineCol(TOK_LEFT_SHIFT, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_LESS, NULL, line, col);
        }

        case '>': {
            if(match(lex, ">=")) {
                return makeLexemLineCol(TOK_GREATER_EQUAL, NULL, line, col);
            }

            if(match(lex, ">>")) {
                return makeLexemLineCol(TOK_RIGHT_SHIFT, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_GREATER, NULL, line, col);
        }
        case '.': {
            if(match(lex, "...")) {
                return makeLexemLineCol(TOK_DOTDOTDOT, NULL, line, col);
            }
            incLexer(lex);
            return makeLexemLineCol(TOK_DOT, NULL, line, col);
        }

        case '?':
            incLexer(lex);
            return makeLexemLineCol(TOK_NULLABLE, NULL, line, col);
        case ':':
            incLexer(lex);
            return makeLexemLineCol(TOK_COLON, NULL, line, col);
        case ';':
            incLexer(lex);
            return makeLexemLineCol(TOK_SEMICOLON, NULL, line, col);
        case '(':
            incLexer(lex);
            return makeLexemLineCol(TOK_LPAREN, NULL, line, col);
        case ')':
            incLexer(lex);
            return makeLexemLineCol(TOK_RPAREN, NULL, line, col);
        case '[':
            incLexer(lex);
            return makeLexemLineCol(TOK_LBRACKET, NULL, line, col);
        case ']':
            incLexer(lex);
            return makeLexemLineCol(TOK_RBRACKET, NULL, line, col);
        case '{':
            incLexer(lex);
            return makeLexemLineCol(TOK_LBRACE, NULL, line, col);
        case '}':
            incLexer(lex);
            return makeLexemLineCol(TOK_RBRACE, NULL, line, col);
        case ',':
            incLexer(lex);
            return makeLexemLineCol(TOK_COMMA, NULL, line, col);

        default: {
            if(getCurrentChar(lex) == '\0'){
                return makeLexemLineCol(TOK_EOF, NULL, line, col);
            }
            if(isalpha(c)){
                return lexIdOrKeyword(lex);
            }
            if(match(lex, "0b")) {
                return lexBinaryValue(lex);
            }
            if(match(lex, "0x")) {
                return lexHexValue(lex);
            }
            if(match(lex, "0o")) {
                return lexOctalValue(lex);
            }
            if(c == '"') {
                return  lexString(lex);
            }
            if(c == '\'') {
                return  lexChar(lex);
            }
            if(isnumber(c)) {
                return lexNumber(lex);
            }
        }
    }


}