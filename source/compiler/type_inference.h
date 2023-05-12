//
// Created by praisethemoon on 10.05.23.
//

#ifndef TYPE_C_TYPE_INFERENCE_H
#define TYPE_C_TYPE_INFERENCE_H

#include "ast.h"
#include "parser.h"

DataType* ti_lambda_toType(Parser * parser, ASTScope * currentScope, LambdaExpr* lambda, Lexeme lexeme);
DataType* ti_fndect_toType(Parser * parser, ASTScope * currentScope, FnDeclStatement * fndecl, Lexeme lexeme);

void ti_runProgram(Parser* parser, ASTProgramNode* program);
void ti_runStatement(Parser* parser, ASTScope* currentScope, Statement * stmt);
void ti_runExpr(Parser* parser, ASTScope* currentScope, Expr* expr);

void ti_infer_element(Parser* parser, ASTScope* scope, Expr* expr);
void ti_infer_exprLiteral(Parser* parser, ASTScope* scope, Expr* expr);
void ti_infer_expr(Parser* parser, ASTScope* scope, Expr* expr);

DataType*  ti_cast_check(Parser* parser, ASTScope* currentScope, Expr* expr, DataType* toType);
void ti_cast(Parser* parser, ASTScope* currentScope, Expr* expr, DataType* toType);

uint8_t ti_struct_contains(Parser* parser, ASTScope* currentScope, DataType* bigStruct, DataType* smallStruct);

uint8_t ti_match_types(Parser* parser, ASTScope* currentScope, DataType* left, DataType* right);
#endif //TYPE_C_TYPE_INFERENCE_H
