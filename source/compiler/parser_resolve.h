//
// Created by praisethemoon on 04.05.23.
//

#ifndef TYPE_C_PARSER_RESOLVE_H
#define TYPE_C_PARSER_RESOLVE_H

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

DataType*  resolver_resolveType(Parser* parser, ASTScope* currentScope, char* typeName);

#endif //TYPE_C_PARSER_RESOLVE_H
