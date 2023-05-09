//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_TYPE_CHECKER_H
#define TYPE_C_TYPE_CHECKER_H

#include "ast.h"

uint8_t tc_check_canJoinOrUnion(ASTScope* scope, DataType* a, DataType* b);

#endif //TYPE_C_TYPE_CHECKER_H
