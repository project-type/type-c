//
// Created by praisethemoon on 05.05.23.
//

#ifndef TYPE_C_AST_JSON_H
#define TYPE_C_AST_JSON_H

#include "ast.h"
#include "../utils/parson.h"

// mostly internal:
JSON_Value* ast_json_serializeDataTypeRecursive(DataType* type);
JSON_Value* ast_json_serializeExprRecursive(Expr* expr);
JSON_Value* ast_json_serializeStatementRecursive(Statement* stmt);
char* ast_stringifyBinaryExprType(BinaryExprType type);
char* ast_stringifyUnaryExprType(UnaryExprType type);

char* ast_json_serializeImports(ASTProgramNode* node);
char* ast_json_serializeDataType(DataType* type);
char* ast_json_serializeExpr(Expr* expr);
char* ast_json_serializeStatement(Statement* stmt);

#endif //TYPE_C_AST_JSON_H
