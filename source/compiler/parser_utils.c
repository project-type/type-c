//
// Created by praisethemoon on 09.05.23.
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "parser_utils.h"
#include "parser.h"

void parser_assert(int cond, const char * rawcond, const char* func_name, int line, const char * fmt, ...) {
    if (cond)
        return;
    char temp[1024];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(temp, fmt, vl);
    va_end(vl);
    fprintf(stdout, "Fatal error, assertion failed: `%s` in function `%s`, line %d \n", rawcond, func_name, line);
    fprintf(stdout, "%s", temp);
    fprintf(stdout, "\n");
}


char* extractLine(Parser* parser, Lexeme lexeme){
    uint32_t token_len = lexeme.type != TOK_IDENTIFIER? strlen(token_type_to_string(lexeme.type)) : strlen(lexeme.string);
    char line[512] = {0};
    uint32_t lineIndex1 = lexeme.pos;
    // find new line pre pos:
    while (lineIndex1 > 0 && parser->lexerState->buffer[lineIndex1-1] != '\n') {
        lineIndex1--;
    }
    // find new line post pos:
    uint32_t lineIndex2 = lexeme.pos;
    while (lineIndex2 < parser->lexerState->len && parser->lexerState->buffer[lineIndex2] != '\n') {
        lineIndex2++;
    }
    // create new string
    uint32_t lineLength = lineIndex2 - lineIndex1;
    strncpy(line, parser->lexerState->buffer + lineIndex1, lineLength);
    // add new line
    line[lineLength] = '\n';
    // now we add spaces from new line until pos relative to line
    uint32_t spaces = lexeme.pos - lineIndex1;
    for (uint32_t i = 0; i < spaces; i++) {
        line[lineLength+i+1] = ' ';
    }
    // add ^ equal to token length
    for (uint32_t i = 0; i < token_len; i++) {
        line[lineLength+spaces+i+1] = '^';
    }
    line[lineLength+token_len+spaces+3] = '\0';


    //line[lineLength+spaces+1] = '^';
    //line[lineLength+spaces+2] = '\0';
    return strdup(line);
}

void parser_utils_raise(Parser* parser, Lexeme lexeme, const char* condition, const char* func_name, int line, const char* fmt, ...){
    char temp[1024];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(temp, fmt, vl);
    va_end(vl);

    fprintf(stdout, "%s:%"PRIu32":%"PRIu32": error: %s\n", parser->lexerState->filename, lexeme.line, lexeme.col, temp);
    fprintf(stdout, "%s\n", extractLine(parser, lexeme));
    fprintf(stdout, "triggered from: %s:%d\n", func_name, line);
    fprintf(stdout, "Failure condition: %s\n", condition);
}