//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_TYPE_CHECKER_H
#define TYPE_C_TYPE_CHECKER_H

#include "ast.h"
#include "parser.h"

DataTypeKind tc_gettype_base(Parser* parser, ASTScope* scope, DataType* type);
uint8_t tc_check_canJoinOrUnion(Parser* parser, ASTScope* scope, DataType* left, DataType* right);
char* tc_accumulate_type_methods_attribute(Parser* parser, ASTScope * scope, DataType* type, map_int_t * map);
char* tc_check_canUnion(Parser* parser, ASTScope* scope, DataType* left, DataType* right);
char* tc_check_canJoin(Parser* parser, ASTScope* scope, DataType* left, DataType* right);

#endif //TYPE_C_TYPE_CHECKER_H
