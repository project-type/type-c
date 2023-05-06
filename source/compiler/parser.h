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
ASTNode* parser_parse(Parser* parser);

void parser_parseProgram(Parser* parser, ASTNode* node);
void parser_parseFromStmt(Parser* parser, ASTNode* node, ASTScope* currentScope);
void parser_parseImportStmt(Parser* parser, ASTNode* node, ASTScope* currentScope);
void parser_parseTypeDecl(Parser* parser, ASTNode* node, ASTScope* currentScope);

DataType* parser_parseTypeUnion(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeIntersection(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeGroup(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeArray(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypePrimary(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);

// enums can't have templates and are final
DataType* parser_parseTypeEnum(Parser* parser, ASTNode* node, ASTScope* currentScope);
DataType* parser_parseTypeStruct(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeRef(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeVariant(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeInterface(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeFn(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypePtr(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);

DataType* parser_parseTypeClass(Parser* parser, ASTNode* node, ASTScope* currentScope);

void parser_parseTypeTemplate(Parser* parser, ASTNode* node, DataType* parentType, ASTScope* currentScope);

void parser_parseExtends(Parser* parser, ASTNode* node, DataType* parentType, vec_dtype_t* extends, ASTScope* currentScope);
void parser_parseFnDefArguments(Parser* parser, ASTNode* node, DataType* parentType, map_fnargument_t* args, vec_str_t* argsNames, ASTScope* currentScope);
PackageID* parser_parsePackage(Parser* parser, ASTNode* node, ASTScope* currentScope);
FnHeader* parser_parseFnHeader(Parser* parser, ASTNode* node, ASTScope* currentScope);
FnHeader* parser_parseLambdaFnHeader(Parser* parser, ASTNode* node, DataType* parentReferee, ASTScope* currentScope);
Expr* parser_parseExpr(Parser* parser, ASTNode* node, ASTScope* currentScope);

// let x = y in z
Expr* parser_parseLetExpr(Parser* parser, ASTNode* node, ASTScope* currentScope);

// match z {cases}
Expr* parser_parseMatchExpr(Parser* parser, ASTNode* node, ASTScope* currentScope);

// =, +=, -=, *=, /=
Expr* parser_parseOpAssign(Parser* parser, ASTNode* node, ASTScope* currentScope);

// ||
Expr* parser_parseOpOr(Parser* parser, ASTNode* node, ASTScope* currentScope);

// &&
Expr* parser_parseOpAnd(Parser* parser, ASTNode* node, ASTScope* currentScope);

// |
Expr* parser_parseOpBinOr(Parser* parser, ASTNode* node, ASTScope* currentScope);

// ^
Expr* parser_parseOpBinXor(Parser* parser, ASTNode* node, ASTScope* currentScope);

// &
Expr* parser_parseOpBinAnd(Parser* parser, ASTNode* node, ASTScope* currentScope);

// == !=
Expr* parser_parseOpEq(Parser* parser, ASTNode* node, ASTScope* currentScope);

// < > <= >=, is
Expr* parser_parseOpCompare(Parser* parser, ASTNode* node, ASTScope* currentScope);

// << >>
Expr* parser_parseOpShift(Parser* parser, ASTNode* node, ASTScope* currentScope);

// + -
Expr* parser_parseAdd(Parser* parser, ASTNode* node, ASTScope* currentScope);

// * / %
Expr* parser_parseOpMult(Parser* parser, ASTNode* node, ASTScope* currentScope);

// * ++ -- ! new sizeof
Expr* parser_parseOpUnary(Parser* parser, ASTNode* node, ASTScope* currentScope);


// a.b, a[b], a(b)
Expr* parser_parseOpPointer(Parser* parser, ASTNode* node, ASTScope* currentScope);

// fn (args_list) -> return_type { body }
// fn (args_list) { body }
// fn (args_list) -> return_type = uhs
Expr* parser_parseOpLambda(Parser* parser, ASTNode* node, ASTScope* currentScope);
Expr* parser_parseOpValue(Parser* parser, ASTNode* node, ASTScope* currentScope);
Expr* parser_parseLiteral(Parser* parser, ASTNode* node, ASTScope* currentScope);


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
