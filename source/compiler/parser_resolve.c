//
// Created by praisethemoon on 04.05.23.
//

#include <stdint.h>
#include <stdlib.h>
#include "../utils/map.h"
#include "../utils/vec.h"
#include "parser_resolve.h"
#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "type_inference.h"

DataType* resolver_resolveType(Parser* parser, ASTScope* currentScope, char* typeName) {
    uint8_t fetchParent = 1;
    if(currentScope == NULL) {
        return NULL;
    }

    DataType ** dtptr = map_get(&currentScope->dataTypes, typeName);
    if(dtptr != NULL) {
        return *dtptr;
    }
    else {
        /*
        GenericParam ** param = map_get(&currentScope->generics, typeName);
        if(param != NULL) {
            return ;
        }*/
        return resolver_resolveType(parser, currentScope->parentScope, typeName);
    }


    return NULL;
}

DataType* resolver_resolveStructAttribute(Parser* parser, ASTScope* currentScope, DataType* structType, char* methodName){
    DataType * dt = ti_type_findBase(parser, currentScope, structType);
    ASSERT(dt->kind == DT_STRUCT, "Expected struct type");
    char* att;
    uint32_t i = 0;
    vec_foreach(&dt->structType->attributeNames, att, i) {
        if(strcmp(att, methodName) == 0){
            StructAttribute ** structAttribute = map_get(&dt->structType->attributes, att);
            if(structAttribute != NULL){
                return (*structAttribute)->type;
            }
        }
    }

    return NULL;
}

DataType* resolver_resolveInterfaceMethod(Parser* parser, ASTScope* currentScope, DataType* interfaceType, char* methodName){
    DataType * dt = ti_type_findBase(parser, currentScope, interfaceType);

    ASSERT(dt->kind == DT_INTERFACE, "Expected interface type");

    char* att;
    uint32_t i = 0;
    vec_foreach(&dt->interfaceType->methodNames, att, i) {
        if(strcmp(att, methodName) == 0){
            FnHeader ** fnHeader = map_get(&dt->interfaceType->methods, att);
            if(fnHeader != NULL){
                return ti_fnheader_toType(parser, currentScope, *fnHeader, interfaceType->lexeme);
            }
        }
    }

    if(dt->interfaceType->extends.length > 0){
        DataType* parent;
        i = 0;
        vec_foreach(&dt->interfaceType->extends, parent, i) {
            DataType * parentMethod = resolver_resolveInterfaceMethod(parser, currentScope, parent, methodName);
            if(parentMethod != NULL){
                return parentMethod;
            }
        }
    }

    return NULL;
}

DataType* resolver_resolveClassField(Parser* parser, ASTScope* currentScope, DataType* classType, char* field){
    DataType * dt = ti_type_findBase(parser, currentScope, classType);

    ASSERT(dt->kind == DT_CLASS, "Expected class type");

    // search for the name in class attributes first
    uint32_t i = 0;
    LetExprDecl* let;
    vec_foreach(&dt->classType->letList, let, i){
        char* var;
        uint32_t j = 0;

        vec_foreach(&let->variableNames, var, j){
            if(strcmp(var, field) == 0){
                FnArgument ** arg = map_get(&let->variables, var);
                return (*arg)->type;
            }
        }
    }

    // next look up methods
    char* att;
    vec_foreach(&dt->classType->methodNames, att, i) {
        if(strcmp(att, field) == 0){
            ClassMethod ** method = map_get(&dt->classType->methods, att);
            if(method != NULL){
                return (*method)->decl->dataType;
            }
        }
    }

    // look up parent interfaces
    if(dt->classType->extends.length > 0){
        DataType* parent;
        i = 0;
        vec_foreach(&dt->classType->extends, parent, i) {
            DataType * parentMethod = resolver_resolveInterfaceMethod(parser, currentScope, parent, field);
            if(parentMethod != NULL){
                return parentMethod;
            }
        }
    }

    return NULL;
}

DataType* resolver_resolveClassMethod(Parser* parser, ASTScope* currentScope, DataType* classType, char* field){
    DataType * dt = ti_type_findBase(parser, currentScope, classType);

    ASSERT(dt->kind == DT_CLASS, "Expected class type");

    uint32_t i = 0;
    char* att;
    vec_foreach(&dt->classType->methodNames, att, i) {
            if(strcmp(att, field) == 0){
                ClassMethod ** method = map_get(&dt->classType->methods, att);
                if(method != NULL){
                    return (*method)->decl->dataType;
                }
            }
        }

    // look up parent interfaces
    if(dt->classType->extends.length > 0){
        DataType* parent;
        i = 0;
        vec_foreach(&dt->classType->extends, parent, i) {
                DataType * parentMethod = resolver_resolveInterfaceMethod(parser, currentScope, parent, field);
                if(parentMethod != NULL){
                    return parentMethod;
                }
            }
    }

    return NULL;
}

uint8_t resolver_matchTypes(DataType* t1, DataType* t2){
    return 0;
}