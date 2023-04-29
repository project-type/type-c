#include <stdio.h>
#include <stdlib.h>
#include "compiler/lexer.h"
#include "compiler/parser.h"

int main() {
    char* filename = "../../samples/sample2.tc";
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Could not open file '%s'\n", filename);
        return 1;
    }

    // Get file size
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate buffer for file contents
    char *input = malloc(file_size + 1);
    if (!input)
    {
        fprintf(stderr, "Could not allocate memory for file contents\n");
        return 1;
    }

    // Read file contents into buffer
    size_t bytes_read = fread(input, 1, file_size, file);
    input[bytes_read] = '\0';

    LexerState* lex = lexer_init("sample.tc", input, bytes_read);
    Parser* parser = parser_init(lex);
    parser_parse(parser);
    return 0;
}
