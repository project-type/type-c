cmake_minimum_required(VERSION 3.25)
project(type_c C)

set(CMAKE_C_STANDARD 99)

add_executable(type_c main.c compiler/lexer.c compiler/lexer.h compiler/tokens.h compiler/parser.c compiler/parser.h compiler/ast.c compiler/ast.h utils/vec.c utils/vec.h)
