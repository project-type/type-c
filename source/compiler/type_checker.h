//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_TYPE_CHECKER_H
#define TYPE_C_TYPE_CHECKER_H

#include "ast.h"
DataTypeKind tc_gettype_base(DataType* type);
uint8_t tc_check_canJoinOrUnion(DataType* left, DataType* right);
char* tc_accumulate_type_methods_attribute(DataType* type, map_int_t * map);
char* tc_check_canUnion(DataType* left, DataType* right);
char* tc_check_canJoin(DataType* left, DataType* right);

#endif //TYPE_C_TYPE_CHECKER_H
