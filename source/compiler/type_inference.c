//
// Created by praisethemoon on 10.05.23.
//

#include "type_inference.h"
#include "ast.h"
#include "parser.h"

DataType * ti_lambda_toType(Parser * parser, ASTScope * currentScope, LambdaExpr* lambda, Lexeme lexeme){
    // generate the datatype of a lambda expression
    // create an empty datatype
    DataType* dt = ast_type_makeType(currentScope, lexeme);
    dt->kind = DT_FN;
    dt->fnType = ast_type_makeFn();

    // copy args
    uint32_t i = 0;
    char* argName = NULL;

    vec_foreach(&lambda->header->type->argNames, argName, i){
        vec_push(&dt->fnType->argNames, argName);
        FnArgument ** arg = map_get(&lambda->header->type->args, argName);
        map_set(&dt->fnType->args, argName, arg);
    }

    // compute type
    if((lambda->header->type->returnType == NULL) && (lambda->bodyType == FBT_EXPR)){
        lambda->header->type->returnType = lambda->expr->dataType;
    }

    if(lambda->header->type->returnType){
        // TODO: deep copy?
        dt->fnType->returnType = lambda->header->type->returnType;
    }

    return dt;
}

DataType* ti_fndect_toType(Parser * parser, ASTScope * currentScope, FnDeclStatement * fndecl, Lexeme lexeme){
    // generate the datatype of a function declaration
    // create an empty datatype
    DataType* dt = ast_type_makeType(currentScope, lexeme);
    dt->kind = DT_FN;
    dt->fnType = ast_type_makeFn();

    // copy args
    uint32_t i = 0;
    char* argName = NULL;

    vec_foreach(&fndecl->header->type->argNames, argName, i){
        vec_push(&dt->fnType->argNames, argName);
        FnArgument ** arg = map_get(&fndecl->header->type->args, argName);
        map_set(&dt->fnType->args, argName, *arg);
    }

    // compute type
    if((fndecl->header->type->returnType == NULL) && (fndecl->bodyType == FBT_EXPR)){
        fndecl->header->type->returnType = fndecl->expr->dataType;
    }

    if(fndecl->header->type->returnType){
        dt->fnType->returnType = fndecl->header->type->returnType;
    }

    return dt;
}

void ti_runProgram(Parser* parser, ASTProgramNode);