//
// Created by praisethemoon on 30.04.23.
//

#include <stdlib.h>
#include <stdio.h>
#include "../../utils/minunit.h"
#include "../../utils/vec.h"
#include "../../utils/map.h"
#include "../lexer.h"
#include "../parser.h"
#include "../ast.h"

char* readFile(const char* url){
    char* filename = url; "../../samples/sample2.tc";
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Could not open file '%s'\n", filename);
        return NULL;
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
        return NULL;
    }

    // Read file contents into buffer
    size_t bytes_read = fread(input, 1, file_size, file);
    input[bytes_read] = '\0';

    return input;
}

MU_TEST(test_imports_1){
    char* input = readFile("../../source/compiler/unittest/import.tc");
    LexerState* lex = lexer_init("import.tc", input, strlen(input));
    Parser* parser = parser_init(lex);
    ASTNode* node = parser_parse(parser);
    // make sure that the number of imporst is 8
    mu_assert_int_eq(8, node->programNode->importStatements.length);

    const char* imports[]  = {
        "std.io.lib.console",
        "matplotlib.pyplot.plot as pt",
        "matplotlib.cool",
        "pandas.pandas_api as pd",
        "pandas.pp",
        "x.y.z.cool.z.y as haha",
        "x.y.z.cool.z",
        "x.y.z.cool as yeet"
    };

    for (uint32_t i = 0; i < node->programNode->importStatements.length; i++){
        // get string version
        //char* import = ast_stringifyImport(node->programNode, i);
        //mu_assert_string_eq(imports[i], import);
    }
}

MU_TEST(test_type_declaration_1){


}

MU_TEST(sample_1) {
    char* input = readFile("../../source/compiler/unittest/sample2.tc");
    LexerState* lex = lexer_init("sample2.tc", input, strlen(input));
    Parser* parser = parser_init(lex);
    ASTNode* node = parser_parse(parser);
}

MU_TEST_SUITE(imports_test) {
    MU_RUN_TEST(test_imports_1);
}

MU_TEST_SUITE(type_declaration_test) {
    MU_RUN_TEST(test_type_declaration_1);
}

MU_TEST_SUITE(not_a_test) {
    MU_RUN_TEST(sample_1);
}

int main(int argc, char *argv[]) {
    //MU_RUN_SUITE(imports_test);
    //MU_RUN_SUITE(type_declaration_test);
    MU_RUN_SUITE(not_a_test);
    MU_REPORT();
    return MU_EXIT_CODE;
}