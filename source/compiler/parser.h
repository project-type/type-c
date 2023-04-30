//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_PARSER_H
#define TYPE_C_PARSER_H

#
#include "lexer.h"
#include "ast.h"
#include "../utils/vec.h"

typedef vec_t(Lexeme) lexem_vec_t;

typedef struct Parser {
    LexerState* lexerState;
    lexem_vec_t stack;
    uint32_t stack_index;
}Parser;

Parser* parser_init(LexerState* lexerState);
/**
 * Returns the current lexeme, and caches into its stack
 * The stack can hold as much as needed for look-aheads
 * But they must accepted or rejected.
 * Once rejected, the stack index will restart form 0
 * so it doesn't have to lex again
 * @param parser
 * @return Lexeme next lexeme.
 */
Lexeme parser_peek(Parser* parser);

/**
 * Accept the lexems, at the current stack position.
 * If multiple lexems has been peeked, and some rejected,
 * the rest are still cached and returned by future peeks
 * @param parser
 */
void parser_accept(Parser* parser);

/**
 * Rejects lexems, sets the stack index to 0
 * @param parser
 */
void parser_reject(Parser* parser);

/**
 * Cooking starts here
 * @param parser
 */
void parser_parse(Parser* parser);

void parser_parseProgram(Parser* parser, ASTNode* node);
void parser_parseFromStmt(Parser* parser, ASTNode* node);
void parser_parseImportStmt(Parser* parser, ASTNode* node);
void parser_parseTypeDecl(Parser* parser, ASTNode* node);

DataType* parser_parseTypeUnion(Parser* parser, ASTNode* node);
DataType* parser_parseTypeIntersection(Parser* parser, ASTNode* node);
DataType* parser_parseTypeGroup(Parser* parser, ASTNode* node);
DataType* parser_parseTypeArray(Parser* parser, ASTNode* node);
DataType* parser_parseTypePrimary(Parser* parser, ASTNode* node);

DataType* parser_parseTypeRegular(Parser* parser, ASTNode* node);
ArrayType* parser_parseArrayType(Parser* parser, ASTNode* node);

EnumType* parser_parseTypeEnum(Parser* parser, ASTNode* node);
PackageID* parser_parsePackage(Parser* parser, ASTNode* node);

#endif //TYPE_C_PARSER_H

/*
<type_definition> ::= "type" <identifier> "=" <union_type>
<union_type> ::= <intersection_type> ( "|" <union_type> )*
<intersection_type> ::= <group_type> ( "&" <intersection_type> )*
<group_type> ::= <array_type> | "(" <union_type> ")"
<array_type> ::= <primary_type> "[" "]"
               | <primary_type> "[" <integer_literal> "]"


<type_definition> ::= "type" <identifier> "=" <union_type>
<union_type> ::= <intersection_type> ( "|" <union_type> )*
<intersection_type> ::= <array_type> ( "&" <intersection_type> )*
<array_type> ::= <group_type> ("[" <int>? "]")*
<group_type> ::= <primary_type> | "(" <union_type> ")"



<primary_type> ::= <interface_type>
                 | <enum_type>
                 | <struct_type>
                 | <data_type>
                 | <function_type>
                 | <basic_type>
                 | <ptr_type>
                 | <class_type>
                 | <reference_type>
 */
