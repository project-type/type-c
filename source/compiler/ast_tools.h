//
// Created by praisethemoon on 05.05.23.
//

#ifndef TYPE_C_AST_TOOLS_H
#define TYPE_C_AST_TOOLS_H

#include "ast.h"

char* ast_stringifyType(DataType* type);
char* ast_stringifyImport(ASTProgramNode* node, uint32_t index);
char* ast_stringifyExpr(Expr* expr);

#endif //TYPE_C_AST_TOOLS_H
