//
// Created by praisethemoon on 10.05.23.
//
#include <assert.h>

#include "parser_resolve.h"
#include "type_inference.h"
#include "ast.h"
#include "parser.h"
#include "error.h"
#include "parser_utils.h"
#include "scope.h"
#include "ast_json.h"

DataType* ti_type_findBase(Parser* parser, ASTScope * scope, DataType *dtype){
    // if type is reference, lookup the scope for the reference
    if(dtype->kind == DT_REFERENCE){
        DataType * dt = NULL;
        if (dtype->refType->ref != NULL)
            dt = dtype->refType->ref;
        else {
            //dt = map_get(&scope->dataTypes, dtype->refType->pkg->ids.data[0]);
             DataType* tmp = resolver_resolveType(parser, scope, dtype->refType->pkg->ids.data[0]);
             if(tmp != NULL)
                dt = tmp;
        }
        if(dt != NULL) {
            if(dt->kind == DT_REFERENCE){
                return ti_type_findBase(parser, scope, dt);
            }
            return dt;
        }

        else {
            Lexeme lexeme = dtype->lexeme;
            PARSER_ASSERT(0, "Could not find reference to type %s", dtype->refType->pkg->ids.data[0])
        }
    }
    return dtype;
}

DataType * ti_fnheader_toType(Parser * parser, ASTScope * currentScope, FnHeader * header, Lexeme lexeme){
    // generate the datatype of a lambda expression
    // create an empty datatype
    DataType* dt = ast_type_makeType(currentScope, lexeme, DT_FN);
    dt->fnType = ast_type_makeFn();

    // copy args
    uint32_t i = 0;
    char* argName = NULL;

    vec_foreach(&header->type->argNames, argName, i){
        vec_push(&dt->fnType->argNames, argName);
        FnArgument ** arg = map_get(&header->type->args, argName);
        map_set(&dt->fnType->args, argName, arg);
    }

    // compute type
    if(header->type->returnType){
        // TODO: deep copy?
        dt->fnType->returnType = header->type->returnType;
    }

    return dt;
}

DataType* ti_fndecl_toType(Parser * parser, ASTScope * currentScope, FnDeclStatement * fndecl, Lexeme lexeme){
    // generate the datatype of a function declaration
    // create an empty datatype
    DataType* dt = ast_type_makeType(currentScope, lexeme, DT_FN);
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
            ti_infer_expr(parser, currentScope, stmt->expr->expr);
            break;
        case ST_VAR_DECL:
            // todo check if variables has types, else we set it to the type of the expression
            break;
        case ST_FN_DECL:
            // todo check if function has return type or infer it from body/expr
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
            // TODO: check if return has an expression
            break;
        case ST_BREAK:
            break;
        case ST_UNSAFE:
            ti_runStatement(parser, currentScope, stmt->unsafeStmt->block);
            break;
        case ST_SYNC:
            ti_runStatement(parser, currentScope, stmt->syncStmt->block);
            break;
        case ST_SPAWN:
            break;
        case ST_EMIT:
            break;
    }
}

void ti_infer_exprLiteral(Parser* parser, ASTScope* scope, Expr* expr){
    // literal types are already setup in the parser
    ASSERT(expr->dataType != NULL, "Literal type not set");
}

void ti_infer_exprThis(Parser* parser, ASTScope* scope, Expr* expr){
    ASSERT(scope->withinClass, "`this` can only be used inside a class");
    // find class reference
    DataType * dt = scope_getClassRef(scope);
    ASSERT(dt != NULL, "Could not find class reference");
    expr->dataType = dt;
}

void ti_infer_element(Parser* parser, ASTScope* scope, Expr* expr) {
    char* name = expr->elementExpr->name;
    ASTScopeResult* res = resolveElement(name, scope, 1);
    ASSERT(res != NULL, "Element %s not found", name);

    if(res->type == SCOPE_VARIABLE){
        expr->dataType = ti_type_findBase(parser, scope, res->variable->type);
    }
    if(res->type == SCOPE_FUNCTION){
        //expr->dataType = ti_type_findBase(parser, scope, ti_fndecl_toType(res->function->));
        expr->dataType = ti_fnheader_toType(parser, scope, res->function, expr->lexeme);
        uint32_t i = 0;
        //expr->dataType = res->dataType;
    }
}

void ti_infer_exprArrayConstruction(Parser* parser, ASTScope* scope, Expr* expr){
    ArrayConstructionExpr* arrExpr = expr->arrayConstructionExpr;
    DataType* elementType = NULL;
    if(arrExpr->args.length > 0){
        // infer the type of the first argument
        ti_infer_expr(parser, scope, arrExpr->args.data[0]);
        // set the type of the array to the type of the first argument
        elementType = arrExpr->args.data[0]->dataType;

        Expr* arg; uint32_t i =0;
        vec_foreach(&arrExpr->args, arg, i) {
            ti_infer_expr(parser, scope, arg);
            Lexeme lexeme = arg->lexeme;
            elementType = ti_types_getCommonType(parser, scope, elementType, arg->dataType);
            PARSER_ASSERT(elementType != NULL, "Array construction type mismatch");
        }

    } else {
        expr->dataType = NULL;
    }

}

void ti_infer_expr(Parser* parser, ASTScope* scope, Expr* expr) {
    switch(expr->type){
        case ET_LITERAL:
            ti_infer_exprLiteral(parser, scope, expr);
            break;
        case ET_THIS:
            ti_infer_exprThis(parser, scope, expr);
            break;
        case ET_ELEMENT:
            ti_infer_element(parser, scope, expr);
            break;
        case ET_ARRAY_CONSTRUCTION:
            ti_infer_exprArrayConstruction(parser, scope, expr);
            break;
        case ET_NAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_UNNAMED_STRUCT_CONSTRUCTION:
            break;
        case ET_NEW:
            expr->dataType = expr->newExpr->type;
            break;
        case ET_CALL:
            ti_infer_expr(parser, scope, expr->callExpr->lhs);
            expr->dataType = ti_call_check(parser, scope, expr);
            break;
        case ET_MEMBER_ACCESS: {
            //printf("%s\n", ast_json_serializeExpr(expr));
            ti_infer_expr(parser, scope, expr->memberAccessExpr->lhs);
            //ti_infer_expr(parser, scope, expr->memberAccessExpr->rhs);
            DataType * res = ti_member_access_check(parser, scope, expr->memberAccessExpr->lhs,
                                                    expr->memberAccessExpr->rhs);
            // check if lhs expression has field rhs
            Lexeme lexeme = expr->memberAccessExpr->rhs->lexeme;
            PARSER_ASSERT(res!=NULL, "Field %s not exist on lhs expression", expr->memberAccessExpr->rhs->elementExpr->name);
            expr->dataType = res;
            break;
        }

        case ET_INDEX_ACCESS: {
            // infer main expression
            ti_infer_expr(parser, scope, expr->indexAccessExpr->expr);

            // infer indexes
            uint32_t i = 0;
            Expr* indexExpr;
            vec_foreach(&expr->indexAccessExpr->indexes, indexExpr, i){
                ti_infer_expr(parser, scope, indexExpr);
            }

            // index access works inherently on arrays, strings but also on objects with __index__ method
            DataType* res = ti_index_access_check(parser, scope, expr->indexAccessExpr->expr, expr->indexAccessExpr->indexes);
            Lexeme lexeme = expr->lexeme;
            PARSER_ASSERT(res!=NULL, "Index access requires either an array or class/interface with `__index__` method");

            break;
        }
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
            ti_infer_expr(parser, expr->unsafeExpr->scope, expr->unsafeExpr->expr);
            expr->dataType = expr->unsafeExpr->expr->dataType;
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

DataType* ti_index_access_check(Parser* parser, ASTScope* currentScope, Expr* expr, vec_expr_t indexes){
    DataType* dt = ti_type_findBase(parser, currentScope, expr->dataType);
    Lexeme lexeme = expr->lexeme;

    if(dt->kind == DT_ARRAY) {
        // make sure we have only one index expression
        PARSER_ASSERT(indexes.length == 1, "Index access on array requires exactly one index expression");
        // make sure the datatype of the index is a valid positive int
        Expr* indexExpr = indexes.data[0];
        PARSER_ASSERT(indexExpr->dataType->kind <= DT_U64, "Index access on array requires an integer index");

        return dt->arrayType->arrayOf;
    }
    else {
        // check if the type has a __index__ method
        if(dt->kind == DT_INTERFACE)
            return resolver_resolveInterfaceMethod(parser, currentScope, dt, "__index__");
        else if(dt->kind == DT_CLASS)
            return resolver_resolveClassMethod(parser, currentScope, dt, "__index__");
    }

    return NULL;
}

DataType* ti_member_access_check(Parser* parser, ASTScope* currentScope, Expr* expr, Expr* element){
    // checks if the expr has a field defined in element and returns the type of the field

    // assert element is of type ElementExpr
    ASSERT(element->type == ET_ELEMENT, "Expected element expression");
    char* name = element->elementExpr->name;
    // assert expr->dataType is either a struct, class or interface
    DataType* dt = ti_type_findBase(parser, currentScope, expr->dataType);
    Lexeme lexeme = expr->lexeme;
    PARSER_ASSERT((dt->kind == DT_STRUCT) || (dt->kind == DT_CLASS) || (dt->kind == DT_INTERFACE), "Expected struct, interface or class type as lhs of member access ");

    if(dt->kind == DT_STRUCT) {
        return resolver_resolveStructAttribute(parser, currentScope, dt, name);
    }

    if(dt->kind == DT_INTERFACE) {
        return resolver_resolveInterfaceMethod(parser, currentScope, dt, name);
    }

    if(dt->kind == DT_CLASS){
        return resolver_resolveClassField(parser, currentScope, dt, name);
    }

    return NULL;
}

DataType* ti_cast_check(Parser* parser, ASTScope* currentScope, Expr* expr, DataType* toType) {
    // check if we can cast expr to toType
    Lexeme lexeme = toType->lexeme;

    // we try the easy approaches first
    if((expr->dataType->kind < DT_F64) && (toType->kind < DT_F64)){
        return toType;
    }

    DataType* fromType = ti_type_findBase(parser, currentScope, expr->dataType);
    DataType* targetType = ti_type_findBase(parser, currentScope, toType);

    if((fromType->kind == DT_STRUCT) && (targetType->kind == DT_STRUCT)) {
        if(ti_struct_contains(parser, currentScope, fromType, targetType)){
            return toType;
        }
    }

    // we swap because match types checks if left is compatible with right, not the other way around.
    // this is important for example if x = y, we check match_types(x.type, y.type)
    PARSER_ASSERT(ti_types_match(parser, currentScope, targetType, fromType), "Cannot cast %s to %s", stringifyType(fromType), stringifyType(targetType));
    return NULL;
}

DataType* ti_call_check(Parser* parser, ASTScope* currentScope, Expr* expr){
    // make sure the lhs is a function
    DataType* lhsType = ti_type_findBase(parser, currentScope, expr->callExpr->lhs->dataType);
    Lexeme lexeme=expr->lexeme;
    PARSER_ASSERT(lhsType->kind == DT_FN, "Cannot call non-function type %s", stringifyType(lhsType));

    return expr->callExpr->lhs->dataType->fnType->returnType;
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

        PARSER_ASSERT(ti_types_match(parser, currentScope, (*attrS)->type, (*attrB)->type), "Structs do not match, attribute `%s` missing");
    }
}

uint8_t ti_types_match(Parser* parser, ASTScope* currentScope, DataType* left, DataType* right){
    // check if any of left or right is a reference
    DataType* L = NULL;
    DataType* R = NULL;

    if (left->kind == DT_REFERENCE) {
        L = ti_type_findBase(parser, currentScope, left);
    } else {
        L = left;
    }

    if(right->kind == DT_REFERENCE){
        R = ti_type_findBase(parser, currentScope, right);
    } else {
        R = right;
    }

    if((L->kind == DT_STRUCT) && (R->kind == DT_STRUCT)){
        return ti_struct_contains(parser, currentScope, L, R);
    }

    if((L->kind == DT_INTERFACE) && (R->kind == DT_CLASS)){
        // check if the class right extends left
        // iterate through right class extends
        DataType * parentType;
        uint32_t i=0;
        vec_foreach(&right->classType->extends, parentType, i){
            if(ti_types_match(parser, currentScope, L, parentType))
                return 1;
        }

        // else we the cast is only possible in unsafe block
        Lexeme lexeme = left->lexeme;
        if(currentScope->isSafe) {
            PARSER_ASSERT(0, "Cannot cast unrelated interface to class outside `unsafe` expression/block");
        }
        else
            return 1;
    }

    if((L->kind == DT_CLASS) && (R->kind == DT_INTERFACE)){
        if (!currentScope->isSafe){
            return 1;
        }
        Lexeme lexeme = left->lexeme;
        //PARSER_ASSERT(0, "Cannot cast class to interface outside `unsafe` expression/block");
        printf("Warning: Cannot cast class to interface outside `unsafe` expression/block\n");
        return 0;
    }

    if(L->kind != R->kind){
        return 0;
    }

    return 1;
}

DataType* ti_types_getCommonType(Parser* parser, ASTScope* currentScope, DataType* left, DataType* right) {
    if(ti_types_match(parser, currentScope, left, right)){
        return left;
    }
    else if (ti_types_match(parser, currentScope, right, left)){
        return right;
    }

    return NULL;
}