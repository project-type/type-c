cmake_minimum_required(VERSION 3.25)
project(type_c C)

set(CMAKE_C_STANDARD 99)

add_executable(type_c main.c compiler/lexer.c compiler/lexer.h compiler/tokens.h compiler/parser.c compiler/parser.h compiler/ast.c compiler/ast.h utils/vec.c utils/vec.h compiler/error.c compiler/error.h utils/map.c utils/map.h compiler/tokens.c compiler/unittest/unittest.c utils/minunit.h compiler/parser_resolve.c compiler/parser_resolve.h compiler/ast_json.c compiler/ast_json.h utils/sds.c utils/sds.h utils/parson.c utils/parson.h utils/sdsalloc.h)
