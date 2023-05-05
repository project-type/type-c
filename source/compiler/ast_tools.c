//
// Created by praisethemoon on 05.05.23.
//

#include <stdio.h>
#include "ast_tools.h"
#include "ast.h"
#include "error.h"
#include "ast_json.h"

char* ast_stringifyImport(ASTProgramNode* node, uint32_t index) {
    // returns string representation of the import statement at the given index
    // we concatenate the path and add "as" alias at the end if it has one
    ImportStmt* importStmt = node->importStatements.data[index];
    char* str = strdup("");
    uint32_t i; char** val;
    vec_foreach_ptr(&importStmt->path->ids, val, i) {
        str = realloc(str, strlen(str) + strlen(*val) + 1);
        strcat(str, *val);
        if(i != importStmt->path->ids.length - 1){
            str = realloc(str, strlen(str) + 1);
            strcat(str, ".");
        }
    }
    if(importStmt->hasAlias){
        str = realloc(str, strlen(str) + strlen(" as ") + strlen(importStmt->alias) + 1);
        strcat(str, " as ");
        strcat(str, importStmt->alias);
    }

    return str;
}

// returns a string representation of the type
char* ast_stringifyType(DataType* type){
    // create base string
    char* str = strdup("");
    if(type->name != NULL){
        str = realloc(str, strlen(str) + strlen(type->name) + 1);
        strcat(str, type->name);
    }
    // if nullable, we add ?
    if(type->isNullable){
        str = realloc(str, strlen(str) + strlen("?") + 1);
        strcat(str, "?");
    }
    // we check its kind
    switch (type->kind) {
        case DT_UNRESOLVED:
            str = realloc(str, strlen(str) + strlen("unresolved") + 1);
            strcat(str, "unresolved");
            break;
        case DT_I8:
            str = realloc(str, strlen(str) + strlen("i8") + 1);
            strcat(str, "i8");
            break;
        case DT_I16:
            str = realloc(str, strlen(str) + strlen("i16") + 1);
            strcat(str, "i16");
            break;
        case DT_I32:
            str = realloc(str, strlen(str) + strlen("i32") + 1);
            strcat(str, "i32");
            break;

        case DT_I64:
            str = realloc(str, strlen(str) + strlen("i64") + 1);
            strcat(str, "i64");
            break;
        case DT_U8:
            str = realloc(str, strlen(str) + strlen("u8") + 1);
            strcat(str, "u8");
            break;
        case DT_U16:
            str = realloc(str, strlen(str) + strlen("u16") + 1);
            strcat(str, "u16");
            break;
        case DT_U32:
            str = realloc(str, strlen(str) + strlen("u32") + 1);
            strcat(str, "u32");
            break;
        case DT_U64:
            str = realloc(str, strlen(str) + strlen("u64") + 1);
            strcat(str, "u64");
            break;
        case DT_F32:
            str = realloc(str, strlen(str) + strlen("f32") + 1);
            strcat(str, "f32");
            break;
        case DT_F64:
            str = realloc(str, strlen(str) + strlen("f64") + 1);
            strcat(str, "f64");
            break;
        case DT_BOOL:
            str = realloc(str, strlen(str) + strlen("bool") + 1);
            strcat(str, "bool");
            break;
        case DT_CHAR:
            str = realloc(str, strlen(str) + strlen("char") + 1);
            strcat(str, "char");
            break;
        case DT_STRING:
            str = realloc(str, strlen(str) + strlen("string") + 1);
            strcat(str, "string");
            break;
        case DT_ARRAY:
            // we print it in the form of array<type, size>
            str = realloc(str, strlen(str) + strlen("array") + 1);
            strcat(str, "array");
            str = realloc(str, strlen(str) + strlen("<") + 1);
            strcat(str, "[");
            char* arrayType = ast_stringifyType(type->arrayType->arrayOf);
            str = realloc(str, strlen(str) + strlen(arrayType) + 1);
            strcat(str, arrayType);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            char* arraySize = malloc(20);
            sprintf(arraySize, "%llu", type->arrayType->len);
            str = realloc(str, strlen(str) + strlen(arraySize) + 1);
            strcat(str, arraySize);
            str = realloc(str, strlen(str) + strlen(">") + 1);
            strcat(str, "]");

            break;
        case DT_ENUM:
            str = realloc(str, strlen(str) + strlen("enum") + 1);
            strcat(str, "enum");
            // we add all fields in the form of enum{field1, field2, field3}
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->enumType->enumNames, val, i) {
        str = realloc(str, strlen(str) + strlen(val) + 1);
        strcat(str, val);
        str = realloc(str, strlen(str) + strlen(",") + 1);
        strcat(str, ",");
    }
            // replace last , with }
            str[strlen(str) - 1] = '}';

            break;
        case DT_REFERENCE:
            // we check if package is null, if it's not we add it to the string as <p1.p2.p3>
            if(type->refType->pkg != NULL){
                str = realloc(str, strlen(str) + strlen("ref(") + 1);
                strcat(str, "ref(");
                // we loop
                int i; char * val;
                vec_foreach(&type->refType->pkg->ids, val, i) {
                    str = realloc(str, strlen(str) + strlen(val) + 1);
                    strcat(str, val);
                    str = realloc(str, strlen(str) + strlen(".") + 1);
                    strcat(str, ".");
                }
                // we replace the last . with >
                str[strlen(str) - 1] = ')';
            }
            else {
                // we throw an error if the ref is nul
                if(type->refType->ref == NULL){
                    printf("Error: reference type is null\n");
                    exit(1);
                }

                // we add the name of the reference type, or "anynomous" if the name is null
                if(type->refType->ref->name == NULL){
                    str = realloc(str, strlen(str) + strlen("anynomous") + 1);
                    strcat(str, "anynomous");
                }
                else {
                    str = realloc(str, strlen(str) + strlen(type->refType->ref->name) + 1);
                    strcat(str, type->refType->ref->name);
                }
            }
            break;
        case DT_TYPE_UNION: {
            // we print union in the form of union{type1, type2, type3}
            str = realloc(str, strlen(str) + strlen("union") + 1);
            strcat(str, "union");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i;
            char *val;
            // we stringify lhs and rhs DataTypes of the union
            char *left = ast_stringifyType(type->unionType->left);
            char *right = ast_stringifyType(type->unionType->right);
            // we concatenate them separate by ","
            str = realloc(str, strlen(str) + strlen(left) + 1);
            strcat(str, left);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            str = realloc(str, strlen(str) + strlen(right) + 1);
            strcat(str, right);
            // close "}"
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");

            break;
        }
        case DT_TYPE_JOIN: {
            // same as we did to union but with join
            str = realloc(str, strlen(str) + strlen("join") + 1);
            strcat(str, "join");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i;
            char *val;
            // we stringify lhs and rhs DataTypes of the union
            char *left = ast_stringifyType(type->joinType->left);
            char *right = ast_stringifyType(type->joinType->right);
            // we concatenate them separate by "&"
            str = realloc(str, strlen(str) + strlen(left) + 1);
            strcat(str, left);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            str = realloc(str, strlen(str) + strlen(right) + 1);
            strcat(str, right);
            // close "}"
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");
            break;

        }
        case DT_STRUCT:{
            // we add all struct fields to string
            str = realloc(str, strlen(str) + strlen("struct") + 1);
            strcat(str, "struct");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->structType->attributeNames, val, i) {
                // we add the name of the attribute
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by :
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                // followed by the type of the attribute
                // first get attribute type from the map
                StructAttribute ** attrType = map_get(&type->structType->attributes, val);
                // then stringify it
                char * attrTypeStr = ast_stringifyType((*attrType)->type);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(attrTypeStr) + 1);
                strcat(str, attrTypeStr);
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");

            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_VARIANT: {
            // we write it as variant{ConstructorName1(arg1: type1, arg2: type2), ConstructorName2(arg1: type1, arg2: type2) , etc)}
            str = realloc(str, strlen(str) + strlen("variant") + 1);
            strcat(str, "variant");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->variantType->constructorNames, val, i) {
                // we add the name of the constructor
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by (
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                // followed by the arguments
                int j; char * argName;
                // get current constructor
                VariantConstructor ** constructor = map_get(&type->variantType->constructors, val);
                // iterate through the arguments
                vec_foreach(&(*constructor)->argNames, argName, j) {
                    // add the name of the argument
                    str = realloc(str, strlen(str) + strlen(argName) + 1);
                    strcat(str, argName);
                    // followed by :
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // followed by the type of the argument
                    // first get the argument type from the map
                    VariantConstructorArgument ** argType = map_get(&(*constructor)->args, argName);
                    // then stringify it
                    char * argTypeStr = ast_stringifyType((*argType)->type);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                    strcat(str, argTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_INTERFACE: {
            // we write it as interface{FunctionName1(arg1: type1, arg2: type2) -> returnType1, FunctionName2(arg1: type1, arg2: type2) -> returnType2 , etc)}
            str = realloc(str, strlen(str) + strlen("interface") + 1);
            strcat(str, "interface");
            // we add ":" followed "(" <list of extends> ")" if there are any
            if (type->interfaceType->extends.length > 0) {
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                int i; DataType * val;
                vec_foreach(&type->interfaceType->extends, val, i) {
                    // we add the name of the interface
                    char* parentTypeStr = ast_stringifyType(val);
                    str = realloc(str, strlen(str) + strlen(parentTypeStr) + 1);
                    strcat(str, parentTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
            }
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->interfaceType->methodNames, val, i) {
                // we add the name of the function
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by (
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                // followed by the arguments
                int j; char * argName;
                // get current function
                FnHeader ** function = map_get(&type->interfaceType->methods, val);
                // iterate through the arguments
                vec_foreach(&(*function)->type->argNames, argName, j) {
                    // add the name of the argument
                    str = realloc(str, strlen(str) + strlen(argName) + 1);
                    strcat(str, argName);
                    // followed by :
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // followed by the type of the argument
                    // first get the argument type from the map
                    FnArgument ** argType = map_get(&(*function)->type->args, argName);
                    // then stringify it
                    char * argTypeStr = ast_stringifyType((*argType)->type);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                    strcat(str, argTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
                // followed by -> if return type is not NULL
                if((*function)->type->returnType != NULL){
                    str = realloc(str, strlen(str) + strlen("->") + 1);
                    strcat(str, "->");
                    // followed by the return type
                    // first get the return type from the map
                    char * returnType = ast_stringifyType((*function)->type->returnType);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(returnType) + 1);
                    strcat(str, returnType);
                }
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_FN: {
            // return fn(arg1: type1, arg2: type2) -> returnType
            str = realloc(str, strlen(str) + strlen("fn") + 1);
            strcat(str, "fn");
            // followed by (
            str = realloc(str, strlen(str) + strlen("(") + 1);
            strcat(str, "(");
            // followed by the arguments
            int i; char * argName;
            vec_foreach(&type->fnType->argNames, argName, i) {
                // add the name of the argument
                str = realloc(str, strlen(str) + strlen(argName) + 1);
                strcat(str, argName);
                // followed by :
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                // followed by the type of the argument
                // first get the argument type from the map
                FnArgument ** argType = map_get(&type->fnType->args, argName);
                // then stringify it
                char * argTypeStr = ast_stringifyType((*argType)->type);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                strcat(str, argTypeStr);
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // if we had args, we replace last , with )
            if(type->fnType->argNames.length > 0){
                str[strlen(str) - 1] = ')';
            } else {
                // if we didn't have args, we just add )
                str = realloc(str, strlen(str) + strlen(")") + 1);
                strcat(str, ")");
            }

            // add return type if it exists
            if(type->fnType->returnType != NULL){
                // followed by -> if return type is not NULL
                str = realloc(str, strlen(str) + strlen("->") + 1);
                strcat(str, "->");
                // followed by the return type
                // first get the return type from the map
                char * returnType = ast_stringifyType(type->fnType->returnType);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(returnType) + 1);
                strcat(str, returnType);
            }
            break;
        }
        case DT_PTR:{
            // return ptr<type>
            str = realloc(str, strlen(str) + strlen("ptr") + 1);
            strcat(str, "ptr");
            // followed by <
            str = realloc(str, strlen(str) + strlen("<") + 1);
            strcat(str, "<");
            // followed by the type
            char * ptrType = ast_stringifyType(type->ptrType->target);
            str = realloc(str, strlen(str) + strlen(ptrType) + 1);
            strcat(str, ptrType);
            // followed by >
            str = realloc(str, strlen(str) + strlen(">") + 1);
            strcat(str, ">");
            break;
        }
        default:
            break;
    }

    // we check if it has generics
    if(type->hasGeneric){
        // we add the generics to the string
        int i; GenericParam * val;
        // first we add a "<" to the string
        str = realloc(str, strlen(str) + 1);
        strcat(str, "<");
        // then we add the generics
        vec_foreach(&type->genericParams, val, i) {
            // we check if the genericParam is generic or not
            if(val->isGeneric){
                // if it is, we add the name of the genericParam
                str = realloc(str, strlen(str) + strlen(val->name) + 1);
                strcat(str, val->name);
                if(val->constraint != NULL) {
                    // add ":"
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // if it is not, we add the type of the genericParam
                    char * genericType = ast_stringifyType(val->constraint);
                    str = realloc(str, strlen(str) + strlen(genericType) + 1);
                    strcat(str, genericType);
                }
            } else {
                // if it's not generic then it is a concrete type, we serialize its type
                char * genericType = ast_stringifyType(val->type);
                str = realloc(str, strlen(str) + strlen(genericType) + 1);
                strcat(str, genericType);
            }
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with >
        str[strlen(str) - 1] = '>';

    }
    return str;
}

char* ast_strigifyExpr(Expr* expr){
    char * str = malloc(2);
    strcat(str, "(");
    str[1] = '\0';
    
    // check if its a let
    if(expr->type == ET_LET){
        LetExpr * let = expr->letExpr;
        // prepare let(<id>":"<type>, <id2>":"<type2>, ...) "=" <uhs>

        
        // add let
        str = realloc(str, strlen(str) + strlen("let") + 1);
        strcat(str, "let");
        // followed by (
        str = realloc(str, strlen(str) + strlen("(") + 1);
        strcat(str, "(");
        // followed by the arguments
        int i; LetExprDecl * letDecl;
        uint32_t counter = 0;
        // each let contains multiple declarations
        // each declaration declare one or many vars,
        // when printing, we print them flat
        // first iterate over the declarations
        vec_foreach(&let->letList, letDecl, i) {
            // iterate through the decl vars
            int j; char * var;
            vec_foreach(&letDecl->variableNames, var, j) {
                counter++;
                // get the variable var from the map
                FnArgument ** varDecl = map_get(&letDecl->variables, var);
                // print the variable name
                str = realloc(str, strlen(str) + strlen(var) + 1);
                strcat(str, var);
                // followed by :
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                // followed by the type
                char * varType = ast_stringifyType((*varDecl)->type);
                str = realloc(str, strlen(str) + strlen(varType) + 1);
                strcat(str, varType);
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // remove last , if we had more than one var in the decl
            if(letDecl->variableNames.length > 1){
                str[strlen(str) - 1] = ' ';
            }
            // add =
            str = realloc(str, strlen(str) + strlen("=") + 1);
            strcat(str, "=");
            // add the expression
            char * declExpr = ast_strigifyExpr(letDecl->initializer);
            str = realloc(str, strlen(str) + strlen(declExpr) + 1);
            strcat(str, declExpr);
        }
        // replace last , with ) if we had more than two vars previously
        if(counter > 1){
            str[strlen(str) - 1] = ')';
        } else {
            // if we didn't have args, we just add )
            str = realloc(str, strlen(str) + strlen(")") + 1);
            strcat(str, ")");
        }

        // we print in { <uhs> }
        // add in {
        str = realloc(str, strlen(str) + strlen(" in {") + 1);
        strcat(str, " in {");
        // add the uhs
        char * letExpr = ast_strigifyExpr(let->inExpr);
        str = realloc(str, strlen(str) + strlen(letExpr) + 1);
        strcat(str, letExpr);
        // add in }
        str = realloc(str, strlen(str) + strlen("}") + 1);
        strcat(str, "}");


    }
        // match
    else if (expr->type == ET_MATCH){
        // we print match(<uhs>:(condition1 => expr1, condition2 => expr2, ...))
        MatchExpr * match = expr->matchExpr;
        // prepare match(<uhs>:(condition1 => expr1, condition2 => expr2, ...))
        
        // add match
        str = realloc(str, strlen(str) + strlen("match") + 1);
        strcat(str, "match");
        // followed by (
        str = realloc(str, strlen(str) + strlen("(") + 1);
        strcat(str, "(");
        // iterate over the cases
        int i; CaseExpr * matchCase;
        vec_foreach(&match->cases, matchCase, i) {
            // add the condition
            char * conditionStr = ast_strigifyExpr(matchCase->condition);
            str = realloc(str, strlen(str) + strlen(conditionStr) + 1);
            strcat(str, conditionStr);
            // followed by =>
            str = realloc(str, strlen(str) + strlen("=>") + 1);
            strcat(str, "=>");
            // followed by the expression
            char * exprStr = ast_strigifyExpr(matchCase->expr);
            str = realloc(str, strlen(str) + strlen(exprStr) + 1);
            strcat(str, exprStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ) if length > 1
        if(match->cases.length > 0){
            str[strlen(str) - 1] = ')';
        } else {
            // if we didn't have cases, we just add )
            str = realloc(str, strlen(str) + strlen(")") + 1);
            strcat(str, ")");
        }

    }
        // literal
    else if (expr->type == ET_LITERAL){
        LiteralExpr * lit = expr->literalExpr;
        // prepare <type> <value>
        
        // add the type
        // followed by the value
        char * valueStr = lit->value;
        str = realloc(str, strlen(str) + strlen(valueStr) + 1);
        strcat(str, valueStr);
    }
    else if (expr->type == ET_ELEMENT) {
        // print the name
        ElementExpr *elem = expr->elementExpr;
        
        // add the name
        str = realloc(str, strlen(str) + strlen(elem->name) + 1);
        strcat(str, elem->name);
    }
    else if(expr->type == ET_NEW){
        // new
        NewExpr * new = expr->newExpr;
        // prepare new <type>(<args>)
        
        // add new
        str = realloc(str, strlen(str) + strlen("new") + 1);
        strcat(str, "new");
        // followed by the type
        char * typeStr = ast_stringifyType(new->type);
        str = realloc(str, strlen(str) + strlen(typeStr) + 1);
        strcat(str, typeStr);
        // followed by (
        str = realloc(str, strlen(str) + strlen("(") + 1);
        strcat(str, "(");
        // followed by the args
        int i; Expr * arg;
        vec_foreach(&new->args, arg, i) {
            char * argStr = ast_strigifyExpr(arg);
            str = realloc(str, strlen(str) + strlen(argStr) + 1);
            strcat(str, argStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ) if length > 1
        if(new->args.length > 0){
            str[strlen(str) - 1] = ')';
        } else {
            // if we didn't have args, we just add )
            str = realloc(str, strlen(str) + strlen(")") + 1);
            strcat(str, ")");
        }

    }
    else if(expr->type == ET_MEMBER_ACCESS){
        // print lhs"."rhs
        MemberAccessExpr * memberAccess = expr->memberAccessExpr;
        // prepare <lhs>.<rhs>
        
        // add the lhs
        char * lhsStr = ast_strigifyExpr(memberAccess->lhs);
        str = realloc(str, strlen(str) + strlen(lhsStr) + 1);
        strcat(str, lhsStr);
        // followed by .
        str = realloc(str, strlen(str) + strlen(".") + 1);
        strcat(str, ".");
        // followed by the rhs string
        char * rhsStr = ast_strigifyExpr(memberAccess->rhs);
        str = realloc(str, strlen(str) + strlen(rhsStr) + 1);
        strcat(str, rhsStr);
    }
    else if (expr->type == ET_CALL){
        // lhs "(" <args> ")"
        CallExpr * call = expr->callExpr;
        // prepare <lhs>(<args>)
        
        // add the lhs
        char * lhsStr = ast_strigifyExpr(call->lhs);
        str = realloc(str, strlen(str) + strlen(lhsStr) + 1);
        strcat(str, lhsStr);
        // followed by (
        str = realloc(str, strlen(str) + strlen("(") + 1);
        strcat(str, "(");
        // followed by the args
        int i; Expr * arg;
        vec_foreach(&call->args, arg, i) {
            char * argStr = ast_strigifyExpr(arg);
            str = realloc(str, strlen(str) + strlen(argStr) + 1);
            strcat(str, argStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ) if length > 1
        if(call->args.length > 0){
            str[strlen(str) - 1] = ')';
        } else {
            // if we didn't have args, we just add )
            str = realloc(str, strlen(str) + strlen(")") + 1);
            strcat(str, ")");
        }
    }
    else if (expr->type == ET_INDEX_ACCESS){
        // lhs "[" <index> "]"
        IndexAccessExpr * indexAccess = expr->indexAccessExpr;
        // prepare <lhs>[<index>]
        
        // add the lhs
        char * lhsStr = ast_strigifyExpr(indexAccess->expr);
        str = realloc(str, strlen(str) + strlen(lhsStr) + 1);
        strcat(str, lhsStr);
        // followed by [
        str = realloc(str, strlen(str) + strlen("[") + 1);
        strcat(str, "[");
        // iterate through the indexes
        int i; Expr * index;
        vec_foreach(&indexAccess->indexes, index, i) {
            char * indexStr = ast_strigifyExpr(index);
            str = realloc(str, strlen(str) + strlen(indexStr) + 1);
            strcat(str, indexStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ] if length > 1
        if(indexAccess->indexes.length > 0){
            str[strlen(str) - 1] = ']';
        } else {
            // if we didn't have indexes, we just add ]
            str = realloc(str, strlen(str) + strlen("]") + 1);
            strcat(str, "]");
        }

    }
    else if (expr->type == ET_CAST){
        // <expr> "as" <type> ")"
        CastExpr * cast = expr->castExpr;
        // prepare <expr> as <type>
        
        // add the expr
        char * exprStr = ast_strigifyExpr(cast->expr);
        str = realloc(str, strlen(str) + strlen(exprStr) + 1);
        strcat(str, exprStr);
        // followed by as
        str = realloc(str, strlen(str) + strlen("<as>") + 1);
        strcat(str, "<as>");
        // followed by the type
        char * typeStr = ast_stringifyType(cast->type);
        str = realloc(str, strlen(str) + strlen(typeStr) + 1);
        strcat(str, typeStr);
    }
    else if (expr->type == ET_INSTANCE_CHECK) {
        // <expr> "<is>" <type>
        InstanceCheckExpr *instanceCheck = expr->instanceCheckExpr;
        // prepare <expr> is <type>
        
        // add the expr
        char *exprStr = ast_strigifyExpr(instanceCheck->expr);
        str = realloc(str, strlen(str) + strlen(exprStr) + 1);
        strcat(str, exprStr);
        // followed by is
        str = realloc(str, strlen(str) + strlen("<is>") + 1);
        strcat(str, "<is>");
        // followed by the type
        char *typeStr = ast_stringifyType(instanceCheck->type);
        str = realloc(str, strlen(str) + strlen(typeStr) + 1);
        strcat(str, typeStr);
    }
    else if (expr->type == ET_UNARY){
        // if op is pre <op><expr> else <expr><op>
        UnaryExpr * unary = expr->unaryExpr;
        // check if op is prefix or suffix
        // if op is UET_POST_INC or UET_POST_DEC, it's suffix
        // else it's prefix
        uint8_t isPrefix = unary->type != UET_POST_INC && unary->type != UET_POST_DEC;
        // prepare <op><expr> or <expr><op>
        
        if(isPrefix){
            // convert op to string
            char * opStr = ast_stringifyUnaryExprType(unary->type);
            // add the op to string
            str = realloc(str, strlen(str) + strlen(opStr) + 1);
            strcat(str, opStr);
        }
        // add the expr
        char * exprStr = ast_strigifyExpr(unary->uhs);
        str = realloc(str, strlen(str) + strlen(exprStr) + 1);
        strcat(str, exprStr);
        if(!isPrefix){
            // convert op to string
            char * opStr = ast_stringifyUnaryExprType(unary->type);
            // add the op to string
            str = realloc(str, strlen(str) + strlen(opStr) + 1);
            strcat(str, opStr);
        }
    }
    else if (expr->type == ET_BINARY){
        // add <lhs> <op> <rhs>
        BinaryExpr * binary = expr->binaryExpr;
        // prepare <lhs> <op> <rhs>
        
        // add the lhs
        char * lhsStr = ast_strigifyExpr(binary->lhs);
        str = realloc(str, strlen(str) + strlen(lhsStr) + 1);
        strcat(str, lhsStr);
        // add the op
        char * opStr = ast_stringifyBinaryExprType(binary->type);
        str = realloc(str, strlen(str) + strlen(opStr) + 1);
        strcat(str, opStr);
        // add the rhs
        char * rhsStr = ast_strigifyExpr(binary->rhs);
        str = realloc(str, strlen(str) + strlen(rhsStr) + 1);
        strcat(str, rhsStr);
    }
    else if (expr->type == ET_ARRAY_CONSTRUCTION){
        // we print "[" <args> "]"
        ArrayConstructionExpr * arrayConstruction = expr->arrayConstructionExpr;
        // add [
        str = realloc(str, strlen(str) + strlen("[") + 1);
        strcat(str, "[");
        // iterate through the args
        int i; Expr * arg;
        vec_foreach(&arrayConstruction->args, arg, i) {
            char * argStr = ast_strigifyExpr(arg);
            str = realloc(str, strlen(str) + strlen(argStr) + 1);
            strcat(str, argStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ] if length > 1
        if(arrayConstruction->args.length > 0){
            str[strlen(str) - 1] = ']';
        } else {
            // if we didn't have args, we just add ]
            str = realloc(str, strlen(str) + strlen("]") + 1);
            strcat(str, "]");
        }
    }
    else if (expr->type == ET_UNNAMED_STRUCT_CONSTRUCTION){
        // add "struct" "{" <args> "}"
        UnnamedStructConstructionExpr * unnamedStructConstruction = expr->unnamedStructConstructionExpr;
        // add struct
        str = realloc(str, strlen(str) + strlen("ustruct") + 1);
        strcat(str, "ustruct");
        // add {
        str = realloc(str, strlen(str) + strlen("{") + 1);
        strcat(str, "{");
        // iterate through the args
        int i; Expr * arg;
        vec_foreach(&unnamedStructConstruction->args, arg, i) {
            char * argStr = ast_strigifyExpr(arg);
            str = realloc(str, strlen(str) + strlen(argStr) + 1);
            strcat(str, argStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with } if length > 1
        if(unnamedStructConstruction->args.length > 0){
            str[strlen(str) - 1] = '}';
        } else {
            // if we didn't have args, we just add }
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");
        }
    }
    else if (expr->type == ET_NAMED_STRUCT_CONSTRUCTION){
        // we add "struct" "{" <name1>":"<arg1>, <name2>":"<arg2>, ... "}"
        NamedStructConstructionExpr * namedStructConstruction = expr->namedStructConstructionExpr;
        // add struct
        str = realloc(str, strlen(str) + strlen("mstruct") + 1);
        strcat(str, "nstruct");
        // add {
        str = realloc(str, strlen(str) + strlen("{") + 1);
        strcat(str, "{");
        // iterate through the args
        int i; char* argName;
        vec_foreach(&namedStructConstruction->argNames, argName, i) {
            // print arg name
            str = realloc(str, strlen(str) + strlen(argName) + 1);
            strcat(str, argName);
            // followed by :
            str = realloc(str, strlen(str) + strlen(":") + 1);
            strcat(str, ":");
            // print arg
            Expr ** arg = map_get(&namedStructConstruction->args, argName);
            char * argStr = ast_strigifyExpr(*arg);
            str = realloc(str, strlen(str) + strlen(argStr) + 1);
            strcat(str, argStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with } if length > 1
        if(namedStructConstruction->argNames.length > 0){
            str[strlen(str) - 1] = '}';
        } else {
            // if we didn't have args, we just add }
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");
        }
    }
    else if (expr->type == ET_LAMBDA){
        // we add "lambda" "(" <args> ")" "{" <body> "}"
        LambdaExpr * lambda = expr->lambdaExpr;
        // add lambda
        str = realloc(str, strlen(str) + strlen("lambda") + 1);
        strcat(str, "lambda");
        // add (
        str = realloc(str, strlen(str) + strlen("(") + 1);
        strcat(str, "(");
        // iterate through the args
        int i; char* argName;
        vec_foreach(&lambda->header->type->argNames, argName, i) {
            // print arg name
            str = realloc(str, strlen(str) + strlen(argName) + 1);
            strcat(str, argName);
            // followed by type
            str = realloc(str, strlen(str) + strlen(":") + 1);
            strcat(str, ":");
            // print arg type
            char * argTypeStr = ast_stringifyType((*map_get(&lambda->header->type->args, argName))->type);
            // add arg type
            str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
            strcat(str, argTypeStr);
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with ) if length > 1
        if(lambda->header->type->argNames.length > 0){
            str[strlen(str) - 1] = ')';
        } else {
            // if we didn't have args, we just add )
            str = realloc(str, strlen(str) + strlen(")") + 1);
            strcat(str, ")");
        }

        // check if its an expression or block
        if(lambda->bodyType == FBT_BLOCK){
            // add {
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");

            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");
        } else {
            // add =
            str = realloc(str, strlen(str) + strlen("=") + 1);
            strcat(str, "=");
            // print body
            char * bodyStr = ast_strigifyExpr(lambda->expr);
            str = realloc(str, strlen(str) + strlen(bodyStr) + 1);
            strcat(str, bodyStr);
        }
    }
    // add )
    str = realloc(str, strlen(str) + strlen(")") + 1);
    strcat(str, ")");

    return str;
}