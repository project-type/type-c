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
        char* import = ast_stringifyImport(node->programNode, i);
        mu_assert_string_eq(imports[i], import);
    }
}

MU_TEST(test_type_declaration_1){
    char* input = readFile("../../source/compiler/unittest/type_decl.tc");
    LexerState* lex = lexer_init("import.tc", input, strlen(input));
    Parser* parser = parser_init(lex);
    ASTNode* node = parser_parse(parser);

    char* arr_dt = "array[array[union{array[u32,255],union{u32,array[ref(f8.cool),255]}},0],0]";
    char* operation_dt = "union{array[enum{ADD,SUB,MUL,DIV,ABS,LOG,EXP,NEG},22],u32}";
    char* operation_null_dt = "?union{array[enum{ADD,SUB,MUL,DIV,ABS,LOG,EXP,NEG},22],?u32}";
    char* userinfo_dt = "array[ref(T)<u32>,255]";
    char* user_dt = "struct{name:string,age:u32,data:union{ref(std.ArrayBuffer),array[ref(std.BinaryBuffer)<ref(String)>,512]}}";
    char* tree_dt = "variant{Leaf(val:u32),Binary(left:ref(Tree),right:ref(Tree)),Unary(child:ref(Tree))}";
    char* serializable_dt = "interface{serialize)->array[u8,0],append(data:array[u8,0]),duplicate(data:ref(Serializable)<array[array[ref(Serializable)<u32>,0],0]>)->ref(Serializable)<u32>}";
    char* serializable2_dt = "interface:(ref(Sortable)<T,ref(String)>){Serialize)->array[ref(T),0],Deserialize(data:array[ref(T),0])}";
    char* fn_dt = "fn(x:ref(T),y:ref(T))->struct{x:ref(T),y:array[ref(T),0]}";
    // make sure we have 9 type declarations
    mu_assert_int_eq(9, node->scope.dataTypes.base.nnodes);

    // extract the type called arr and make sure ist not null
    DataType ** arr = map_get(&node->scope.dataTypes, "arr");
    mu_assert(arr != NULL, "arr is null");
    mu_assert_string_eq(arr_dt, ast_stringifyType((*arr)->refType->ref));

    DataType ** Operation = map_get(&node->scope.dataTypes, "Operation");
    mu_assert(Operation != NULL, "Operation is null");
    mu_assert_string_eq(operation_dt, ast_stringifyType((*Operation)->refType->ref));

    DataType ** OperationNullable = map_get(&node->scope.dataTypes, "OperationNullable");
    mu_assert(OperationNullable != NULL, "OperationNullable is null");
    mu_assert_string_eq(operation_null_dt, ast_stringifyType((*OperationNullable)->refType->ref));

    DataType ** UserInfo = map_get(&node->scope.dataTypes, "UserInfo");
    mu_assert(UserInfo != NULL, "UserInfo is null");
    mu_assert_string_eq(userinfo_dt, ast_stringifyType((*UserInfo)->refType->ref));

    DataType ** user = map_get(&node->scope.dataTypes, "User");
    mu_assert(user != NULL, "User is null");
    mu_assert_string_eq(user_dt, ast_stringifyType((*user)->refType->ref));

    DataType ** Tree = map_get(&node->scope.dataTypes, "Tree");
    mu_assert(Tree != NULL, "Tree is null");
    mu_assert_string_eq(tree_dt, ast_stringifyType((*Tree)->refType->ref));

    DataType ** Serializable = map_get(&node->scope.dataTypes, "Serializable");
    mu_assert(Serializable != NULL, "Serializable is null");
    mu_assert_string_eq(serializable_dt, ast_stringifyType((*Serializable)->refType->ref));

    DataType ** Serializable2 = map_get(&node->scope.dataTypes, "Serializable2");
    mu_assert(Serializable2 != NULL, "Serializable2 is null");
    mu_assert_string_eq(serializable2_dt, ast_stringifyType((*Serializable2)->refType->ref));

    DataType ** Callable = map_get(&node->scope.dataTypes, "Callable");
    mu_assert(Serializable2 != NULL, "Callable is null");
    mu_assert_string_eq(fn_dt, ast_stringifyType((*Callable)->refType->ref));

}

MU_TEST(sample_1) {
    char* input = readFile("../../source/compiler/unittest/sample1.tc");
    LexerState* lex = lexer_init("import.tc", input, strlen(input));
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
    MU_RUN_SUITE(imports_test);
    MU_RUN_SUITE(type_declaration_test);
    MU_RUN_SUITE(not_a_test);
    MU_REPORT();
    return MU_EXIT_CODE;
}