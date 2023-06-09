//
// Created by praisethemoon on 09.05.23.
//

#include "type_checker.h"
#include "parser_resolve.h"
#include "parser_utils.h"
#include "type_inference.h"
#include <assert.h>

DataTypeKind tc_gettype_base(Parser* parser, ASTScope* scope, DataType* type){
    if(type->kind == DT_TYPE_UNION){
        DataTypeKind t1 = tc_gettype_base(parser, scope, type->unionType->left);
        DataTypeKind t2 = tc_gettype_base(parser, scope, type->unionType->left);
        if(t1 != t2){
            return DT_INVALID;
        }
        return t1;
    }

    if(type->kind == DT_TYPE_JOIN){
        DataTypeKind t1 = tc_gettype_base(parser, scope, type->joinType->left);
        DataTypeKind t2 = tc_gettype_base(parser, scope, type->joinType->left);
        if(t1 != t2){
            return DT_INVALID;
        }
        return t1;
    }

    if(type->kind == DT_REFERENCE){
        return type->refType->ref != NULL?tc_gettype_base(parser, scope, type->refType->ref):DT_INVALID;
    }

    return type->kind;
}

char* tc_accumulate_type_methods_attribute(Parser* parser, ASTScope * scope, DataType* type, map_int_t * map){
    if(type->kind == DT_TYPE_UNION){
        map_int_t left;
        map_init(&left);

        char* leftOk = tc_accumulate_type_methods_attribute(parser, scope, type->unionType->left, &left);

        map_int_t right;
        map_init(&right);

        char* rightOk = tc_accumulate_type_methods_attribute(parser, scope, type->unionType->right, &right);

        map_deinit(&left);
        map_deinit(&right);

        return leftOk!=NULL?(leftOk):(rightOk!=NULL?(rightOk):(NULL));
    }

    if(type->kind == DT_TYPE_JOIN){
        char* leftOk = tc_accumulate_type_methods_attribute(parser, scope, type->joinType->left, map);
        char* rightOk = tc_accumulate_type_methods_attribute(parser, scope, type->joinType->right, map);

        return leftOk!=NULL?(leftOk):(rightOk!=NULL?(rightOk):(NULL));
    }

    if(type->kind == DT_STRUCT){
        StructType * structType = type->structType;
        uint32_t i = 0;

        // check parent
        DataType * parent;
        vec_foreach(&structType->extends, parent, i){
            char* parentOk = tc_accumulate_type_methods_attribute(parser, scope, parent, map);
            if(parentOk != NULL){
                return parentOk;
            }
        }

        char* attributeName;
        vec_foreach(&structType->attributeNames, attributeName, i){
            int* val = map_get(map, attributeName);

            if(val != NULL){
                return attributeName;
            }
            map_set(map, attributeName, 1);
        }
    }

    if(type->kind == DT_INTERFACE){
        InterfaceType* interfaceType = type->interfaceType;
        uint32_t i = 0;

        // check parent
        DataType * parent;
        vec_foreach(&interfaceType->extends, parent, i){
            char* parentOk = tc_accumulate_type_methods_attribute(parser, scope, parent, map);
            if(parentOk != NULL){
                return parentOk;
            }
        }

        char* methodName;
        vec_foreach(&interfaceType->methodNames, methodName, i){
            int* val = map_get(map, methodName);

            if(val != NULL){
                return methodName;
            }
            map_set(map, methodName, 1);
        }

    }

    if(type->kind == DT_CLASS){
        ClassType * classType = type->classType;

        // check parent
        uint32_t i = 0;
        DataType * parent;
        vec_foreach(&classType->extends, parent, i){
            char* parentOk = tc_accumulate_type_methods_attribute(parser, scope, parent, map);
            if(parentOk != NULL){
                return parentOk;
            }
        }

        char* methodName;
        vec_foreach(&classType->methodNames, methodName, i){
            int* val = map_get(map, methodName);

            if(val != NULL){
                return methodName;
            }
            map_set(map, methodName, 1);
        }
        LetExprDecl* letDecl;

        vec_foreach(&classType->letList, letDecl, i){
            char* varName; uint32_t j = 0;
            vec_foreach(&letDecl->variableNames, varName, j)
            {
                int *val = map_get(map, varName);

                if (val != NULL) {
                    return varName;
                }
                map_set(map, varName, 1);
            }
        }
    }

    if(type->kind == DT_REFERENCE) {
        if(type->refType->ref != NULL)
            return tc_accumulate_type_methods_attribute(parser, scope, type->refType->ref, map);
        else {
            char* name = type->refType->pkg->ids.data[0];
            DataType* dt = resolver_resolveType(parser, scope, name);
            if(dt == NULL) {
                ASSERT(0, "Type %s not found", name);
            }

            dt = ti_type_findBase(parser, scope, dt);
            return tc_accumulate_type_methods_attribute(parser, scope, dt, map);
        }
        return 0;
    }

    return 0;
}

uint8_t tc_check_canJoinOrUnion(Parser* parser, ASTScope* scope, DataType* left, DataType* right){
    DataTypeKind leftKind = tc_gettype_base(parser, scope, ti_type_findBase(parser, scope, left));
    DataTypeKind rightKind = tc_gettype_base(parser, scope, ti_type_findBase(parser, scope, right));


    if(leftKind == DT_INVALID || rightKind == DT_INVALID){
        return 0;
    }

    if(((leftKind == DT_INTERFACE) || (leftKind == DT_STRUCT)) && (leftKind == rightKind)){
        return 1;
    }

    return 0;
}

char* tc_check_canJoin(Parser* parser, ASTScope* scope, DataType* left, DataType* right) {
    map_int_t map;
    map_init(&map);

    char* leftOk = tc_accumulate_type_methods_attribute(parser, scope, left, &map);
    char* rightOk = tc_accumulate_type_methods_attribute(parser, scope, right, &map);

    map_deinit(&map);
    return leftOk!=NULL?(leftOk):(rightOk!=NULL?(rightOk):(NULL));
}

char* tc_check_canUnion(Parser* parser, ASTScope* scope, DataType* left, DataType* right) {
    map_int_t mapLeft;
    map_init(&mapLeft);
    char* leftOk = tc_accumulate_type_methods_attribute(parser, scope, left, &mapLeft);

    map_int_t mapRight;
    map_init(&mapRight);
    char* rightOk = tc_accumulate_type_methods_attribute(parser, scope, right, &mapRight);

    map_deinit(&mapLeft);
    map_deinit(&mapRight);

    return leftOk!=NULL?(leftOk):(rightOk!=NULL?(rightOk):(NULL));
}

