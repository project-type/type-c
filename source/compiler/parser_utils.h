//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_PARSER_UTILS_H
#define TYPE_C_PARSER_UTILS_H

#include <stdarg.h>
#include "error.h"
#include "parser.h"

void parser_assert(int cond, const char * rawcond, const char* func_name, int line, const char * fmt, ...);
char* extractLine(Parser* parser, Lexeme lexeme);
void parser_utils_raise(Parser* parser, Lexeme lexeme, const char* condition, const char* func_name, int line, const char* fmt, ...);

#define PARSER_ASSERT(c, msg, ...) { \
    if(!(c)){                                     \
        parser_utils_raise(parser, lexeme, #c, __FUNCTION_NAME__, __LINE__ , msg, ##__VA_ARGS__);\
\
        assert(c);                           \
    }                                    \
}

#endif //TYPE_C_PARSER_UTILS_H
