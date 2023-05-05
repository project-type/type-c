//
// Created by praisethemoon on 05.05.23.
//

#ifndef TYPE_C_AST_JSON_H
#define TYPE_C_AST_JSON_H

#include "ast.h"

char* ast_stringifyBinaryExprType(BinaryExprType type);
char* ast_stringifyUnaryExprType(UnaryExprType type);

char* ast_json_serializeImports(ASTProgramNode* node);
char* ast_json_serializeDataType(DataType* type);
#endif //TYPE_C_AST_JSON_H
