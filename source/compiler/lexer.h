//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_LEXER_H
#define TYPE_C_LEXER_H

#include <stdint.h>
#include "tokens.h"

typedef struct Lexem {
    TokenType type;
    char* string;
    uint32_t line;
    uint32_t col;
}Lexem;

typedef struct LexerState {
    const char* filename; /*< Buffer source */
    const char* buffer; /*< Buffer data */
    uint64_t pos;  /*< Buffer pos */
    uint64_t len;  /*< Buffer length */

    uint32_t line; /*< Current line */
    uint32_t col;  /*< Current col */
}LexerState;

LexerState* lexer_init(const char* filename, const char* buffer, uint64_t len);
Lexem lexer_next(LexerState* lexerState);
Lexem lexer_peek(LexerState* lexerState);
void lexer_free(LexerState* lexerState);
Lexem lexer_lexCurrent(LexerState* lexerState);

#endif //TYPE_C_LEXER_H
