//
// Created by praisethemoon on 30.04.23.
//

#include <stdlib.h>
#include <stdio.h>
#include "../../utils/minunit.h"
#include "../lexer.h"
#include "../parser.h"

char* readFile(const char* url){
    char* filename = url; "../../samples/sample2.tc";
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
        char* import = ast_stringifyImport(node->programNode, i);
        mu_assert_string_eq(imports[i], import);
    }
}

MU_TEST(test_type_declaration_1){
    char* input = readFile("../../source/compiler/unittest/type_decl.tc");
    LexerState* lex = lexer_init("import.tc", input, strlen(input));
    Parser* parser = parser_init(lex);
    ASTNode* node = parser_parse(parser);

    char* arr_dt = "array<array<union{array<u32,255>,union{u32,array<<f8.cool>,255>}},0>,0>";
    char* operation_dt = "union{array<enum{ADD,SUB,MUL,DIV,ABS,LOG,EXP,NEG},22>,u32}";
    char* userinfo_dt = "array<<T>,25>";

    // make sure we have 3 type declarations
    mu_assert_int_eq(3, node->scope.dataTypes.base.nnodes);
}

MU_TEST_SUITE(imports_test) {
    MU_RUN_TEST(test_imports_1);
}

MU_TEST_SUITE(type_declaration_test) {
    MU_RUN_TEST(test_type_declaration_1);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(imports_test);
    MU_RUN_SUITE(type_declaration_test);
    MU_REPORT();
    return MU_EXIT_CODE;
}