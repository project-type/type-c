//
// Created by praisethemoon on 09.05.23.
//

#include "type_checker.h"
#include "parser_resolve.h"
#include "parser_utils.h"
#include <assert.h>

DataTypeKind tc_gettype_base(ASTScope* scope, DataType* type){
    if(type->kind == DT_TYPE_UNION){
        DataTypeKind t1 = tc_gettype_base(scope, type->unionType->left);
        DataTypeKind t2 = tc_gettype_base(scope, type->unionType->left);
        if(t1 != t2){
            return DT_INVALID;
        }
        return t1;
    }

    if(type->kind == DT_TYPE_JOIN){
        DataTypeKind t1 = tc_gettype_base(scope, type->joinType->left);
        DataTypeKind t2 = tc_gettype_base(scope, type->joinType->left);
        if(t1 != t2){
            return DT_INVALID;
        }
        return t1;
    }

    if(type->kind == DT_REFERENCE){
        return type->refType->ref != NULL?tc_gettype_base(scope, type->refType->ref):DT_INVALID;
    }

    return type->kind;
}

uint8_t tc_check_canJoinOrUnion(ASTScope* scope, DataType* left, DataType* right) {
    DataTypeKind leftKind = tc_gettype_base(scope, left);
    DataTypeKind rightKind = tc_gettype_base(scope, right);

    printf("left: %d, right: %d\n", leftKind, rightKind);

    if(leftKind == DT_INVALID || rightKind == DT_INVALID){
        return 0;
    }

    if(((leftKind == DT_INTERFACE) || (leftKind == DT_STRUCT)) && (leftKind == rightKind)){
        return 1;
    }

    return 0;
}