//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_PARSER_H
#define TYPE_C_PARSER_H

#
#include "lexer.h"
#include "ast.h"
#include "../utils/vec.h"

typedef vec_t(Lexem) lexem_vec_t;

typedef struct Parser {
    LexerState* lexerState;
    lexem_vec_t stack;
    uint32_t stack_index;
}Parser;

Parser* parser_init(LexerState* lexerState);
/**
 * Returns the current lexem, and caches into its stack
 * The stack can hold as much as needed for look-aheads
 * But they must accepted or rejected.
 * Once rejected, the stack index will restart form 0
 * so it doesn't have to lex again
 * @param parser
 * @return Lexem next lexem.
 */
Lexem parser_peek(Parser* parser);

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
ArrayType* parser_parseArrayType(Parser* parser, ASTNode* node);
EnumType* parser_parseEnumDecl(Parser* parser, ASTNode* node);
PackageID* parser_parsePackage(Parser* parser, ASTNode* node);

#endif //TYPE_C_PARSER_H
