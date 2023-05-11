//
// Created by praisethemoon on 10.05.23.
//

#ifndef TYPE_C_TYPE_INFERENCE_H
#define TYPE_C_TYPE_INFERENCE_H

#include "ast.h"
#include "parser.h"

DataType* ti_lambda_toType(Parser * parser, ASTScope * currentScope, LambdaExpr* lambda, Lexeme lexeme);
DataType* ti_fndect_toType(Parser * parser, ASTScope * currentScope, FnDeclStatement * fndecl, Lexeme lexeme);

void ti_runProgram();

#endif //TYPE_C_TYPE_INFERENCE_H
