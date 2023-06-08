//
// Created by praisethemoon on 04.05.23.
//

#ifndef TYPE_C_PARSER_RESOLVE_H
#define TYPE_C_PARSER_RESOLVE_H

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

/**
 * Resolves a type name in the given scope.
 * @param parser
 * @param currentScope
 * @param typeName
 * @return
 */
DataType* resolver_resolveType(Parser* parser, ASTScope* currentScope, char* typeName);


/**
 * Finds an attribute with the given name in the given struct
 * @param parser
 * @param currentScope
 * @param interfaceType
 * @param methodName
 * @return Attribute type or NULL if not found
 */
DataType* resolver_resolveStructAttribute(Parser* parser, ASTScope* currentScope, DataType* structType, char* methodName);

/**
 * Finds a function with the given name in the given interface
 * Looks in both interface and its parent interfaces
 * @param parser
 * @param currentScope
 * @param interfaceType
 * @param methodName
 * @return Method type or NULL if not found
 */
DataType* resolver_resolveInterfaceMethod(Parser* parser, ASTScope* currentScope, DataType* interfaceType, char* methodName);

/**
 * Resolved a field in a class. The field can be either a method or attribute
 *
 * @param parser
 * @param currentScope
 * @param classType
 * @param methodName
 * @return
 */
DataType* resolver_resolveClassField(Parser* parser, ASTScope* currentScope, DataType* classType, char* methodName);

/**
 * Resolved a method only in given class
 * @param parser
 * @param currentScope
 * @param classType
 * @param field
 * @return
 */
DataType* resolver_resolveClassMethod(Parser* parser, ASTScope* currentScope, DataType* classType, char* field);

#endif //TYPE_C_PARSER_RESOLVE_H
