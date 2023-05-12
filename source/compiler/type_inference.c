//
// Created by praisethemoon on 10.05.23.
//
#include <assert.h>

#include "type_inference.h"
#include "ast.h"
#include "parser.h"
#include "error.h"
#include "parser_utils.h"
#include "scope.h"

DataType* ti_type_findBase(Parser* parser, ASTScope * scope, DataType *dtype){
    // if type is reference, lookup the scope for the reference
    if(dtype->kind == DT_REFERENCE){
        DataType ** dt = NULL;
        if (dtype->refType->ref != NULL)
            dt = &dtype->refType->ref;
        else
            dt = map_get(&scope->dataTypes, dtype->refType->pkg->ids.data[0]);
        if(dt != NULL) {
            if((*dt)->kind == DT_REFERENCE){
                return ti_type_findBase(parser, scope, *dt);
            }
            return *dt;
        }
    }
    return dtype;
}

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

void ti_runProgram(Parser* parser, ASTProgramNode* program) {
    uint32_t i = 0;
    for(; i < program->stmts.length; i++) {
        ti_runStatement(parser, program->scope, program->stmts.data[i]);
    }
}

void ti_runStatement(Parser* parser, ASTScope* currentScope, Statement * stmt){
    switch(stmt->type){
        case ST_EXPR:
            ti_runExpr(parser, currentScope, stmt->expr->expr);
            break;
        case ST_VAR_DECL:
            break;
        case ST_FN_DECL:
            break;
        case ST_BLOCK: {
            // recursively run the statements in the block
            uint32_t i = 0;
            for(; i < stmt->blockStmt->stmts.length; i++) {
                ti_runStatement(parser, stmt->blockStmt->scope, stmt->blockStmt->stmts.data[i]);
            }
            break;
        }
        case ST_IF_CHAIN:
            break;
        case ST_MATCH:
            break;
        case ST_WHILE:
            break;
        case ST_DO_WHILE:
            break;
        case ST_FOR:
            break;
        case ST_FOREACH:
            break;
        case ST_CONTINUE:
            break;
        case ST_RETURN:
            break;
        case ST_BREAK:
            break;
        case ST_UNSAFE:
            break;
        case ST_SYNC:
            break;
        case ST_SPAWN:
            break;
        case ST_EMIT:
            break;
    }
}

void ti_runExpr(Parser* parser, ASTScope* currentScope, Expr* expr) {
    switch (expr->type) {
        case ET_LITERAL:
            break;
        case ET_THIS:
            break;
        case ET_ELEMENT:
            break;
        case ET_ARRAY_CONSTRUCTION:
            break;
        case ET_NAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_UNNAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_NEW:
            break;
        case ET_CALL:
            break;
        case ET_MEMBER_ACCESS:
            break;
        case ET_INDEX_ACCESS:
            break;
        case ET_CAST:
            ti_infer_expr(parser, currentScope, expr);
            break;
        case ET_INSTANCE_CHECK:
            break;
        case ET_UNARY:
            break;
        case ET_BINARY:
            break;
        case ET_IF_ELSE:
            break;
        case ET_MATCH:
            break;
        case ET_LET:
            break;
        case ET_LAMBDA:
            break;
        case ET_UNSAFE:
            break;
        case ET_SYNC:
            break;
        case ET_SPAWN:
            break;
        case ET_EMIT:
            break;
        case ET_WILDCARD:
            break;
    }
}

void ti_infer_exprLiteral(Parser* parser, ASTScope* scope, Expr* expr){
    // literal types are already setup in the parser
    ASSERT(expr->dataType != NULL, "Literal type not set");
}

void ti_infer_element(Parser* parser, ASTScope* scope, Expr* expr) {
    char* name = expr->elementExpr->name;
    ASTScopeResult* res = resolveElement(name, scope, 1);
    ASSERT(res != NULL, "Element %s not found", name);

    if(res->type == SCOPE_VARIABLE){
        expr->dataType = ti_type_findBase(parser, scope, res->variable->type);
    }
}

void ti_infer_expr(Parser* parser, ASTScope* scope, Expr* expr) {
    switch(expr->type){
        case ET_LITERAL:
            ti_infer_exprLiteral(parser, scope, expr);
            break;
        case ET_THIS:
            break;
        case ET_ELEMENT:
            ti_infer_element(parser, scope, expr);
            break;
        case ET_ARRAY_CONSTRUCTION:
            break;
        case ET_NAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_UNNAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_NEW:
            break;
        case ET_CALL:
            break;
        case ET_MEMBER_ACCESS:
            break;
        case ET_INDEX_ACCESS:
            break;
        case ET_CAST:
            ti_infer_expr(parser, scope, expr->castExpr->expr);
            expr->dataType = ti_cast_check(parser, scope, expr->castExpr->expr, expr->castExpr->type);
            break;
        case ET_INSTANCE_CHECK:
            break;
        case ET_UNARY:
            break;
        case ET_BINARY:
            break;
        case ET_IF_ELSE:
            break;
        case ET_MATCH:
            break;
        case ET_LET:
            break;
        case ET_LAMBDA:
            break;
        case ET_UNSAFE:
            break;
        case ET_SYNC:
            break;
        case ET_SPAWN:
            break;
        case ET_EMIT:
            break;
        case ET_WILDCARD:
            break;
    }
}

DataType* ti_cast_check(Parser* parser, ASTScope* currentScope, Expr* expr, DataType* toType) {
    // check if we can cast expr to toType
    Lexeme lexeme = toType->lexeme;

    // we try the easy approaches first
    if((expr->dataType->kind < DT_F64) && (toType->kind < DT_F64)){
        return toType;
    }

    DataType* fromType = ti_type_findBase(parser, currentScope, toType);
    DataType* targetType = ti_type_findBase(parser, currentScope, expr->dataType);

    if((fromType->kind == DT_STRUCT) && (targetType->kind == DT_STRUCT)) {
        if(ti_struct_contains(parser, currentScope, fromType, targetType)){
            return toType;
        }
    }


    PARSER_ASSERT(expr->dataType->kind == targetType->kind, "Cannot cast %s to %s", stringifyType(expr->dataType), stringifyType(toType));
    return NULL;
}

uint8_t ti_struct_contains(Parser* parser, ASTScope* currentScope, DataType* bigStruct, DataType* smallStruct){
    StructType * structB = bigStruct->structType;
    StructType * structS = smallStruct->structType;

    uint32_t i = 0;
    char* attName = NULL;
    vec_foreach(&structS->attributeNames, attName, i){
        StructAttribute** attrS = map_get(&structS->attributes, attName);
        StructAttribute** attrB = map_get(&structB->attributes, attName);
        Lexeme lexeme = bigStruct->lexeme;

        PARSER_ASSERT(ti_match_types(parser, currentScope, (*attrS)->type, (*attrB)->type), "Structs do not match, attribute `%s` missing");
    }
}

uint8_t ti_match_types(Parser* parser, ASTScope* currentScope, DataType* left, DataType* right){

    if((left->kind == DT_STRUCT) && (right->kind == DT_STRUCT)){
        return ti_struct_contains(parser, currentScope, left, right);
    }


    if(left->kind != right->kind){
        return 0;
    }

    return 1;
}