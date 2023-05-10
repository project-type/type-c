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

    vec_dtype_t unresolvedTypes;
    vec_str_t unresolvedSymbols;
    struct ASTProgramNode * programNode;
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

void parser_parseProgram(Parser* parser, ASTProgramNode* node);
void parser_parseFromStmt(Parser* parser, ASTScope* currentScope);
void parser_parseImportStmt(Parser* parser, ASTScope* currentScope);
void parser_parseTypeDecl(Parser* parser, ASTScope* currentScope);

ExternDecl* parser_parseExternDecl(Parser* parser, ASTScope* currentScope);

DataType* parser_parseTypeUnion(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeIntersection(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeGroup(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeArray(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypePrimary(Parser* parser, DataType* parentReferee, ASTScope* currentScope);

// enums can't have templates and are final
DataType* parser_parseTypeEnum(Parser* parser, ASTScope* currentScope);
DataType* parser_parseTypeStruct(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeRef(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeVariant(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeInterface(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeClass(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypeFn(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType* parser_parseTypePtr(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
DataType * parser_parseTypeProcess(Parser* parser, DataType* parentReferee, ASTScope* currentScope);

void parser_parseTypeTemplate(Parser* parser, DataType* parentType, ASTScope* currentScope);

void parser_parseExtends(Parser* parser, DataType* parentType, vec_dtype_t* extends, ASTScope* currentScope, DataTypeKind kind);
void parser_parseFnDefArguments(Parser* parser, DataType* parentType, map_fnargument_t* args, vec_str_t* argsNames, ASTScope* currentScope);
PackageID* parser_parsePackage(Parser* parser, ASTScope* currentScope);
FnHeader* parser_parseFnHeader(Parser* parser, ASTScope* currentScope);
FnHeader* parser_parseLambdaFnHeader(Parser* parser, DataType* parentReferee, ASTScope* currentScope);
Expr* parser_parseExpr(Parser* parser, ASTScope* currentScope);

// let x = y in z
Expr* parser_parseLetExpr(Parser* parser, ASTScope* currentScope);

// match z {cases}
Expr* parser_parseMatchExpr(Parser* parser, ASTScope* currentScope);

// =, +=, -=, *=, /=
Expr* parser_parseOpAssign(Parser* parser, ASTScope* currentScope);

// ||
Expr* parser_parseOpOr(Parser* parser, ASTScope* currentScope);

// &&
Expr* parser_parseOpAnd(Parser* parser, ASTScope* currentScope);

// |
Expr* parser_parseOpBinOr(Parser* parser, ASTScope* currentScope);

// ^
Expr* parser_parseOpBinXor(Parser* parser, ASTScope* currentScope);

// &
Expr* parser_parseOpBinAnd(Parser* parser, ASTScope* currentScope);

// == !=
Expr* parser_parseOpEq(Parser* parser, ASTScope* currentScope);

// < > <= >=, is
Expr* parser_parseOpCompare(Parser* parser, ASTScope* currentScope);

// << >>
Expr* parser_parseOpShift(Parser* parser, ASTScope* currentScope);

// + -
Expr* parser_parseAdd(Parser* parser, ASTScope* currentScope);

// * / %
Expr* parser_parseOpMult(Parser* parser, ASTScope* currentScope);

// * ++ -- ! new sizeof
Expr* parser_parseOpUnary(Parser* parser, ASTScope* currentScope);


// a.b, a[b], a(b)
Expr* parser_parseOpPointer(Parser* parser, ASTScope* currentScope);
// fn (args_list) -> return_type { body }
// fn (args_list) { body }
// fn (args_list) -> return_type = uhs
Expr* parser_parseOpLambda(Parser* parser, ASTScope* currentScope);
Expr* parser_parseOpValue(Parser* parser, ASTScope* currentScope);
Expr* parser_parseLiteral(Parser* parser, ASTScope* currentScope);
Expr* parser_parseGenericFnCall(Parser* parser, ASTScope* currentScope);

Statement* parser_parseStmt(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtLet(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtFn(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtBlock(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtIf(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtMatch(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtWhile(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtDoWhile(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtFor(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtForEach(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtContinue(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtReturn(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtBreak(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtUnsafe(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtSync(Parser* parser, ASTScope* currentScope);
Statement* parser_parseStmtExpr(Parser* parser, ASTScope* currentScope);



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
