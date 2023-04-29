//
// Created by praisethemoon on 28.04.23.
//

#include <stdlib.h>
#include <inttypes.h>
#include <printf.h>
#include "../utils/vec.h"
#include "parser.h"
#include "ast.h"
#include "error.h"
#include "tokens.h"

#define TTTS(t) token_type_to_string(t)

#define ACCEPT parser_accept(parser)
#define CURRENT lexem = parser_peek(parser)
#define EXPAND_LEXEME lexem.line, lexem.col, TTTS(lexem.type)

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
            ACCEPT;
            parser_parseFromStmt(parser, node);
        }
        if(lexem.type == TOK_IMPORT) {
            ACCEPT;
            parser_parseImportStmt(parser, node);
        }

        lexem = parser_peek(parser);
        can_loop = lexem.type == TOK_FROM || lexem.type == TOK_IMPORT;
    }while(can_loop);

    ast_debug_programImport(node->programNode);
    // TODO: Resolve Imports and add them to symbol table
    // we no longer expect import or from after this

    if(lexem.type == TOK_TYPE) {
        parser_parseTypeDecl(parser, node);
    }
}


/*
 * "type" <id> ("<" <generic> ">")
 *    "=" (<id> | <basic_type> | "class" <class_body>
 *              | "interface" <interface| "data" | "enum" | "struct" | "fn")
 */
void parser_parseTypeDecl(Parser* parser, ASTNode* node) {
    ACCEPT;
    Lexem lexem = parser_peek(parser);
    ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" identifier expected but %s was found.", EXPAND_LEXEME);
    const char* name = strdup(lexem.string);
    ACCEPT;

    lexem = parser_peek(parser);
    ASSERT(lexem.type == TOK_EQUAL, "Line: %"PRIu16", Col: %"PRIu16" `=` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    lexem = parser_peek(parser);
    if(lexem.type == TOK_ENUM) {
        EnumType * enum_ = parser_parseEnumDecl(parser, node);
    }
}

/*
 * "enum" "{" <id> (","? <id>)* "}"
 */
EnumType* parser_parseEnumDecl(Parser* parser, ASTNode* node) {
    // current position: enum
    ACCEPT;
    Lexem CURRENT;
    ASSERT(lexem.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
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
    ASSERT(lexem.type == TOK_IMPORT, "Line: %"PRIu16", Col: %"PRIu16" `import` expected but %s was found.", EXPAND_LEXEME);

    ACCEPT;

    uint8_t can_loop = 1;
    do {
        PackageID* target = parser_parsePackage(parser, node);
        lexem = parser_peek(parser);

        uint8_t hasAlias = 0;
        char* alias = NULL;
        // is `as` present?
        if(lexem.type == TOK_TYPE_CONVERSION) {
            hasAlias = 1;
            ACCEPT;
            lexem = parser_peek(parser);
            ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
            alias = strdup(lexem.string);
            ACCEPT;
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
            ACCEPT;
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
        ACCEPT;
        lexem = parser_peek(parser);
        ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        alias = strdup(lexem.string);
        ACCEPT;
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
    ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier/package` expected but %s was found.", EXPAND_LEXEME);
    vec_push(&package->ids, strdup(lexem.string));

    ACCEPT;

    lexem = parser_peek(parser);
    uint8_t can_go = lexem.type == TOK_DOT;
    while(can_go) {
        lexem = parser_peek(parser);
        ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier/package` expected but %s was found.", EXPAND_LEXEME);
        vec_push(&package->ids, strdup(lexem.string));
        ACCEPT;

        lexem = parser_peek(parser);
        can_go = lexem.type == TOK_DOT;
    }

    parser_reject(parser);

    return package;
}

#undef CURRENT
#undef ACCEPT
#undef TTTS