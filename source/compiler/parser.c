//
// Created by praisethemoon on 28.04.23.
//

#include <stdlib.h>
#include <printf.h>
#include "../utils/vec.h"
#include "parser.h"
#include "ast.h"
#include "error.h"

Parser* parser_init(LexerState* lexerState) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexerState = lexerState;
    vec_init(&parser->stack);
    parser->stack_index = 0;
    return parser;
}

Lexem parser_peek(Parser* parser) {

    if (parser->stack_index == parser->stack.length) {
        Lexem lexem = lexer_lexCurrent(parser->lexerState);
        vec_push(&parser->stack, lexem);
        parser->stack_index++;
        return lexem;
    }
    else {
        parser->stack_index++;
        return parser->stack.data[parser->stack_index-1];
    }
}

void parser_accept(Parser* parser) {
    // TODO: free lexem's str value
    vec_splice(&parser->stack, 0, parser->stack_index);
    parser->stack_index = 0;
}

void parser_reject(Parser* parser) {
    parser->stack_index = 0;
}

void parser_parse(Parser* parser) {
    ASTNode* node = ast_makeProgramNode();
    parser_parseProgram(parser, node);
}

void parser_parseProgram(Parser* parser, ASTNode* node) {
    Lexem lexem = parser_peek(parser);

    uint8_t can_loop = lexem.type == TOK_FROM || lexem.type == TOK_IMPORT;

    do {
        if(lexem.type == TOK_FROM) {
            parser_accept(parser);
            parser_parseFromStmt(parser, node);
        }
        if(lexem.type == TOK_IMPORT) {
            parser_accept(parser);
            parser_parseImportStmt(parser, node);
        }

        lexem = parser_peek(parser);
        can_loop = lexem.type == TOK_FROM || lexem.type == TOK_IMPORT;
    }while(can_loop);

    ast_debug_programImport(node->programNode);
}

/*
 * "from" <package_id> "import" <package_id> ("as" <id>)? ("," <package_id> ("as" <id>)?)*
 */
void parser_parseFromStmt(Parser* parser, ASTNode* node){
    // from has already been accepted
    // we expect a namespace x.y.z
    PackageID* source = parser_parsePackage(parser, node);
    Lexem lexem = parser_peek(parser);
    // next we need an import
    ASSERT(lexem.type == TOK_IMPORT,
           "[%zu,%zu]: `import` keyword expected after import <package> but `%d` found",
           lexem.line, lexem.col, lexem.type);
    parser_accept(parser);

    uint8_t can_loop = 1;
    do {
        PackageID* target = parser_parsePackage(parser, node);
        lexem = parser_peek(parser);

        uint8_t hasAlias = 0;
        char* alias = NULL;
        // is `as` present?
        if(lexem.type == TOK_TYPE_CONVERSION) {
            hasAlias = 1;
            parser_accept(parser);
            lexem = parser_peek(parser);
            ASSERT(lexem.type == TOK_IDENTIFIER, "identifier expected after alias `as` but %d was found", lexem.type);
            alias = strdup(lexem.string);
            parser_accept(parser);
        }
        else {
            parser_reject(parser);
        }
        ImportStmt* import = ast_makeImportStmt(source, target, hasAlias, alias);
        vec_push(&node->programNode->importStatements, import);
        lexem = parser_peek(parser);

        // here we might find a comma. if we do, we accept it and keep looping
        can_loop = lexem.type == TOK_COMMA;
        if(can_loop) {
            parser_accept(parser);
        }
    } while (can_loop);
    parser_reject(parser);
}


/*
 * "import" <package_id> ("as" <id>)?
 */
void parser_parseImportStmt(Parser* parser, ASTNode* node){
    // from has already been accepted
    // we expect a namespace x.y.z
    PackageID* source = parser_parsePackage(parser, node);
    uint8_t hasAlias = 0;
    char* alias = NULL;
    Lexem lexem = parser_peek(parser);
    // is `as` present?
    if(lexem.type == TOK_TYPE_CONVERSION) {
        hasAlias = 1;
        parser_accept(parser);
        lexem = parser_peek(parser);
        ASSERT(lexem.type == TOK_IDENTIFIER, "identifier expected after alias `as` but %d was found", lexem.type);
        alias = strdup(lexem.string);
        parser_accept(parser);
    }
    else{
        parser_reject(parser);
    }

    PackageID * empty = ast_makePackageID();
    ImportStmt* import = ast_makeImportStmt(source, empty, hasAlias, alias);
    // TODO: free empty
    vec_push(&node->programNode->importStatements, import);
}


/*
 * Terminal parsers
*/

PackageID* parser_parsePackage(Parser* parser, ASTNode* node) {
    PackageID * package = ast_makePackageID();
    Lexem lexem = parser_peek(parser);
    ASSERT(lexem.type == TOK_IDENTIFIER, "[%zu, %zu] Identifier/package expected after `from` keyword, but `%d` found", lexem.line, lexem.col, lexem.type);
    vec_push(&package->ids, strdup(lexem.string));

    parser_accept(parser);

    lexem = parser_peek(parser);
    uint8_t can_go = lexem.type == TOK_DOT;
    while(can_go) {
        lexem = parser_peek(parser);
        ASSERT(lexem.type == TOK_IDENTIFIER, "[%zu, %zu] Identifier/package expected after `from` keyword, but `%d` found", lexem.line, lexem.col, lexem.type);
        vec_push(&package->ids, strdup(lexem.string));
        parser_accept(parser);

        lexem = parser_peek(parser);
        can_go = lexem.type == TOK_DOT;
    }

    parser_reject(parser);

    return package;
}