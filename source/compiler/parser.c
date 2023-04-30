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

    can_loop = lexem.type == TOK_TYPE;
    while (can_loop) {

        if(lexem.type == TOK_TYPE) {
            parser_parseTypeDecl(parser, node);
            CURRENT;
        }
        else {
            can_loop = 0;
        }
    }
}


/*
 * "type" <id> ("<" <generic> ">")
 *    "=" (<id> | <basic_type> | "class" <class_body>
 *              | "interface" <interface| "data" | "enum" | "struct" | "fn")
 */
void parser_parseTypeDecl(Parser* parser, ASTNode* node) {
    DataType * type = ast_type_makeType();
    ACCEPT;
    Lexem lexem = parser_peek(parser);
    ASSERT(lexem.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" identifier expected but %s was found.", EXPAND_LEXEME);
    char* name = strdup(lexem.string);
    ACCEPT;

    uint8_t isGeneric = 0;

    lexem = parser_peek(parser);

    if(lexem.type == TOK_LESS) {
        // GENERICS!
        type->isGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        uint32_t idx = 0;
        ASSERT(lexem.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);

        while(can_loop) {
            ASSERT(lexem.type == TOK_IDENTIFIER,
                   "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.",
                   EXPAND_LEXEME);

            // make sure same name doesn't appear twice
            if (map_get(&type->generics, lexem.string) != NULL) {
                ASSERT(1==0,
                       "Line: %"PRIu16", Col: %"PRIu16", generic name duplicated: %s",
                       lexem.line, lexem.col, lexem.string);
            }
            vec_push(&type->genericNames, lexem.string);
            map_set(&type->generics, lexem.string, idx++);

            ACCEPT;
            CURRENT;
            if(lexem.type == TOK_GREATER){
                can_loop = 0;
                ACCEPT;
                CURRENT;
            }

            else {
                ASSERT(lexem.type == TOK_COMMA,
                       "Line: %"PRIu16", Col: %"PRIu16" `,` expected but %s was found.",
                       EXPAND_LEXEME);
                ACCEPT;
                CURRENT;
            }
        }
    }

    ASSERT(lexem.type == TOK_EQUAL, "Line: %"PRIu16", Col: %"PRIu16" `=` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    lexem = parser_peek(parser);

    if(lexem.type == TOK_ENUM) {
        EnumType * enum_ = parser_parseEnumDecl(parser, node);
        type->kind = DT_ENUM;
        type->enumType = enum_;
    }

    if((lexem.type >= TOK_I8) && (lexem.type <=TOK_CHAR)) {
        type->kind = lexem.type - TOK_I8;
    }

    if(lexem.type == TOK_IDENTIFIER) {
        // expect an entire package
        parser_reject(parser);
        PackageID * pkg = parser_parsePackage(parser, node);
        ReferenceType * ref = ast_type_makeReference();
        ref->pkg = pkg;
        type->refType = ref;
        type->kind = DT_REFERENCE;
    }

    // post-types, arrays.


    CURRENT;
    uint8_t can_loop = lexem.type == TOK_LBRACKET;

    while(can_loop) {
        // it is an array :(
        ArrayType* array = parser_parseArrayType(parser, node);
        array->arrayOf = type;

        DataType * arr_type = ast_type_makeType();
        arr_type->kind = DT_ARRAY;
        arr_type->arrayType = array;
        type = arr_type;
        CURRENT;
        can_loop = lexem.type == TOK_LBRACKET;
    }

    parser_reject(parser);
    type->name = name;
    map_set(&node->scope.dataTypes, type->name, type);
    ast_debug_Type(type);

}
/*
 * "[" <int>? "]"
 */
ArrayType* parser_parseArrayType(Parser* parser, ASTNode* node) {
    ArrayType* array = ast_type_makeArray();
    // currently at [
    ACCEPT;
    Lexem CURRENT;
    if (lexem.type == TOK_INT) {
        array->len = strtoul(lexem.string, NULL, 10);
        ACCEPT;
        CURRENT;
    }
    else if (lexem.type == TOK_BINARY_INT) {
        array->len = strtoul(lexem.string, NULL, 2);
        ACCEPT;
        CURRENT;
    }
    else if (lexem.type == TOK_OCT_INT) {
        array->len = strtoul(lexem.string, NULL, 8);
        ACCEPT;
        CURRENT;
    }
    else if (lexem.type == TOK_HEX_INT) {
        array->len = strtoul(lexem.string, NULL, 16);
        ACCEPT;
        CURRENT;
    }

    ASSERT(lexem.type == TOK_RBRACKET, "Line: %"PRIu16", Col: %"PRIu16" `]` or `integer constant` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    return array;
}

/*
 * "enum" "{" <id> (","? <id>)* "}"
 */
EnumType* parser_parseEnumDecl(Parser* parser, ASTNode* node) {
    EnumType* enum_ = ast_type_makeEnum();
    // current position: enum
    ACCEPT;
    Lexem CURRENT;
    ASSERT(lexem.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;

    uint8_t can_loop = 1;
    uint32_t index = 0;

    while(can_loop) {
        ASSERT(lexem.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        char* name = lexem.string;
        // TODO: make sure index doesn't exeed some limit?

        if (map_get(&enum_->enums, name) != NULL) {
            ASSERT(1==0,
                   "Line: %"PRIu16", Col: %"PRIu16", enum value duplicated: %s",
                   lexem.line, lexem.col, lexem.string);
        }
        map_set(&enum_->enums, name, index++);
        vec_push(&enum_->enumNames, name);
        printf("pushing %"PRIu32": %s\n", index-1, name);

        ACCEPT;
        CURRENT;

        if(lexem.type == TOK_COMMA){
            ACCEPT;
            CURRENT;
        }

        if(lexem.type != TOK_IDENTIFIER) {
            can_loop = 0;
        }
    }
    ASSERT(lexem.type == TOK_RBRACE, "Line: %"PRIu16", Col: %"PRIu16" `}` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    return enum_;
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