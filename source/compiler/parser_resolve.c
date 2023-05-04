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

DataType* parser_resolveType(Parser* parser, ASTNode* node, ASTScope currentScope, char* typeName) {
    uint8_t fetchParent = 1;
    if(currentScope.parentScope == NULL) {
        fetchParent = 0;
    }

    DataType ** dtptr = map_get(&currentScope.dataTypes, typeName);
    if(dtptr != NULL) {
        return *dtptr;
    }
    else {
        if(fetchParent) {
            parser_resolveType(parser, node, *currentScope.parentScope, typeName);
        }
        else{
            return NULL;
        }
    }
}