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
#define CURRENT lexeme = parser_peek(parser)
#define EXPAND_LEXEME lexeme.line, lexeme.col, TTTS(lexeme.type)

Parser* parser_init(LexerState* lexerState) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexerState = lexerState;
    vec_init(&parser->stack);
    parser->stack_index = 0;
    return parser;
}

Lexeme parser_peek(Parser* parser) {
    if (parser->stack_index == parser->stack.length) {
        Lexeme lexeme = lexer_lexCurrent(parser->lexerState);
        vec_push(&parser->stack, lexeme);
        parser->stack_index++;
        return lexeme;
    }
    else {
        parser->stack_index++;
        return parser->stack.data[parser->stack_index-1];
    }
}

void parser_accept(Parser* parser) {
    // TODO: free lexeme's str value
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
    Lexeme lexeme = parser_peek(parser);

    uint8_t can_loop = lexeme.type == TOK_FROM || lexeme.type == TOK_IMPORT;

    do {
        if(lexeme.type == TOK_FROM) {
            ACCEPT;
            parser_parseFromStmt(parser, node);
        }
        if(lexeme.type == TOK_IMPORT) {
            ACCEPT;
            parser_parseImportStmt(parser, node);
        }

        lexeme = parser_peek(parser);
        can_loop = lexeme.type == TOK_FROM || lexeme.type == TOK_IMPORT;
    }while(can_loop);

    ast_debug_programImport(node->programNode);
    // TODO: Resolve Imports and add them to symbol table
    // we no longer expect import or from after this

    can_loop = lexeme.type == TOK_TYPE;
    while (can_loop) {

        if(lexeme.type == TOK_TYPE) {
            parser_parseTypeDecl(parser, node);
            CURRENT;
        }
        else {
            can_loop = 0;
        }
    }
}

/*
We need to generate functions to parse each of the following:

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
void parser_parseTypeDecl(Parser* parser, ASTNode* node) {
    DataType * type = ast_type_makeType();
    type->kind = DT_REFERENCE;
    ACCEPT;
    Lexeme lexeme = parser_peek(parser);
    ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" identifier expected but %s was found.", EXPAND_LEXEME);
    type->name = strdup(lexeme.string);
    ACCEPT;

    uint8_t isGeneric = 0;

    lexeme = parser_peek(parser);

    if(lexeme.type == TOK_LESS) {
        // GENERICS!
        type->isGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        uint32_t idx = 0;
        ASSERT(lexeme.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);

        while(can_loop) {
            ASSERT(lexeme.type == TOK_IDENTIFIER,
                   "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.",
                   EXPAND_LEXEME);

            // make sure same name doesn't appear twice
            if (map_get(&type->generics, lexeme.string) != NULL) {
                ASSERT(1==0,
                       "Line: %"PRIu16", Col: %"PRIu16", generic name duplicated: %s",
                       lexeme.line, lexeme.col, lexeme.string);
            }
            vec_push(&type->genericNames, lexeme.string);
            map_set(&type->generics, lexeme.string, idx++);

            ACCEPT;
            CURRENT;
            if(lexeme.type == TOK_GREATER){
                can_loop = 0;
                ACCEPT;
                CURRENT;
            }

            else {
                ASSERT(lexeme.type == TOK_COMMA,
                       "Line: %"PRIu16", Col: %"PRIu16" `,` expected but %s was found.",
                       EXPAND_LEXEME);
                ACCEPT;
                CURRENT;
            }
        }
    }

    ASSERT(lexeme.type == TOK_EQUAL, "Line: %"PRIu16", Col: %"PRIu16" `=` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    //lexeme = parser_peek(parser);
    DataType* type_def = parser_parseTypeUnion(parser, node);
    type->refType = ast_type_makeReference();
    type->refType->ref = type_def;

    char* desc = ast_strigifyType(type_def);
    printf(desc);
    printf("\n");
}

// <union_type> ::= <intersection_type> ( "|" <union_type> )*
DataType* parser_parseTypeUnion(Parser* parser, ASTNode* node) {
    // must parse union type
    DataType* type = parser_parseTypeIntersection(parser, node);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_OR) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeUnion(parser, node);
        // create intersection type
        UnionType * unions = ast_type_makeUnion();
        vec_push(&unions->unions, type);
        vec_push(&unions->unions, type2);

        // create new datatype to hold joints
        DataType* newType = ast_type_makeType();
        newType->kind = DT_TYPE_UNION;
        newType->unionType = unions;
        return newType;
    }
    parser_reject(parser);
    return type;
}

// <intersection_type> ::= <group_type> ( "&" <group_type> )*
DataType* parser_parseTypeIntersection(Parser* parser, ASTNode* node){
    // must parse group type
    DataType* type = parser_parseTypeArray(parser, node);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_AND) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeIntersection(parser, node);
        // create intersection type
        JoinType * join = ast_type_makeJoin();
        vec_push(&join->joins, type);
        vec_push(&join->joins, type2);

        // create new datatype to hold joints
        DataType* newType = ast_type_makeType();
        newType->kind = DT_TYPE_JOIN;
        newType->joinType = join;
        return newType;
    }
    parser_reject(parser);
    return type;
}

// <group_type> ::= <primary_type> | "(" <union_type> ")"
DataType* parser_parseTypeGroup(Parser* parser, ASTNode* node) {
    // check if we have group first
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_LPAREN) {
        // we have a group
        ACCEPT;
        DataType* type = parser_parseTypeUnion(parser, node);
        lexeme = parser_peek(parser);
        ASSERT(lexeme.type == TOK_RPAREN, "Line: %"PRIu16", Col: %"PRIu16" `)` expected but %s was found.", EXPAND_LEXEME);
        ACCEPT;
        return type;
    }
    parser_reject(parser);
    DataType* type = parser_parseTypePrimary(parser, node);
    return type;
}


// <array_type> ::= <primary_type> "[" "]" | <primary_type> "[" <integer_literal> "]"
DataType * parser_parseTypeArray(Parser* parser, ASTNode* node) {
    // must parse primary type first
    DataType* primary = parser_parseTypeGroup(parser, node);
    Lexeme lexeme = parser_peek(parser);
    DataType * last_type = primary;
    // if next token is "[" then it is an array

    uint8_t can_loop = lexeme.type == TOK_LBRACKET;

    while(can_loop){
        if(lexeme.type == TOK_LBRACKET) {
            // we have found an array
            ArrayType* array = ast_type_makeArray();
            array->arrayOf = last_type;
            ACCEPT;
            lexeme = parser_peek(parser);
            // if next token is "]" then it is an array of unknown size
            if(lexeme.type == TOK_RBRACKET) {
                ACCEPT;
            }
            else{
                uint32_t arrayLen = 0;
                // value must in either int, hex, oct or bin
                ASSERT((lexeme.type == TOK_INT) || (lexeme.type == TOK_HEX_INT) ||
                       (lexeme.type == TOK_OCT_INT) || (lexeme.type == TOK_BINARY_INT),
                       "Line: %"PRIu16", Col: %"PRIu16" `int` expected but %s was found.",
                       EXPAND_LEXEME);
                // parse the value

                if(lexeme.type == TOK_INT) {
                    arrayLen = strtoul(lexeme.string, NULL, 10);
                }
                else if(lexeme.type == TOK_HEX_INT) {
                    arrayLen = strtoul(lexeme.string, NULL, 16);
                }
                else if(lexeme.type == TOK_OCT_INT) {
                    arrayLen = strtoul(lexeme.string, NULL, 8);
                }
                else if(lexeme.type == TOK_BINARY_INT) {
                    arrayLen = strtoul(lexeme.string, NULL, 2);
                }
                array->len = arrayLen;
                ACCEPT;
                lexeme = parser_peek(parser);
                ASSERT(lexeme.type == TOK_RBRACKET,
                       "Line: %"PRIu16", Col: %"PRIu16" `]` expected but %s was found.",
                       EXPAND_LEXEME);
                ACCEPT;
            }

            DataType * retType = ast_type_makeType();
            retType->kind = DT_ARRAY;
            retType->arrayType = array;
            last_type = retType;
            CURRENT;
        }
        else {
            can_loop = 0;
        }
    }

    return last_type;
}

// <primary_type> ::= <enum_type>
DataType* parser_parseTypePrimary(Parser* parser, ASTNode* node) {
    // parse enum if current keyword is enum
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_ENUM) {
        return parser_parseTypeEnum(parser, node);
    }
    // check if current keyword is a basic type
    if((lexeme.type >= TOK_I8) && (lexeme.type <= TOK_CHAR)) {
        // create new type assign basic to it
        DataType* type = ast_type_makeType();
        type->kind = lexeme.type - TOK_I8;
        ACCEPT;
        return type;
    }
    // check if we have an ID, we parse package then
    if(lexeme.type == TOK_IDENTIFIER) {
        // we create a reference type
        DataType* type = ast_type_makeType();
        type->kind = DT_REFERENCE;
        type->refType = ast_type_makeReference();
        // rollback
        parser_reject(parser);
        type->refType->pkg = parser_parsePackage(parser, node);

        return type;
    }
}


// enum_type ::= "enum" "{" <enum_decl> ("," <enum_decl>)* "}"
EnumType* parser_parseTypeEnum(Parser* parser, ASTNode* node) {
    EnumType* enum_ = ast_type_makeEnum();
    // current position: enum
    ACCEPT;
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;

    uint8_t can_loop = 1;
    uint32_t index = 0;

    while(can_loop) {
        ASSERT(lexeme.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        char* name = lexeme.string;
        // TODO: make sure index doesn't exeed some limit?

        if (map_get(&enum_->enums, name) != NULL) {
            ASSERT(1==0,
                   "Line: %"PRIu16", Col: %"PRIu16", enum value duplicated: %s",
                   lexeme.line, lexeme.col, lexeme.string);
        }
        map_set(&enum_->enums, name, index++);
        vec_push(&enum_->enumNames, name);

        ACCEPT;
        CURRENT;

        if(lexeme.type == TOK_COMMA){
            ACCEPT;
            CURRENT;
        }

        if(lexeme.type != TOK_IDENTIFIER) {
            can_loop = 0;
        }
    }
    ASSERT(lexeme.type == TOK_RBRACE, "Line: %"PRIu16", Col: %"PRIu16" `}` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    DataType * enumType = ast_type_makeType();
    enumType->kind = DT_ENUM;
    enumType->enumType = enum_;
    return enumType;
}

/*
 * "from" <package_id> "import" <package_id> ("as" <id>)? ("," <package_id> ("as" <id>)?)*
 */
void parser_parseFromStmt(Parser* parser, ASTNode* node){
    // from has already been accepted
    // we expect a namespace x.y.z
    PackageID* source = parser_parsePackage(parser, node);
    Lexeme lexeme = parser_peek(parser);
    // next we need an import
    ASSERT(lexeme.type == TOK_IMPORT, "Line: %"PRIu16", Col: %"PRIu16" `import` expected but %s was found.", EXPAND_LEXEME);

    ACCEPT;

    uint8_t can_loop = 1;
    do {
        PackageID* target = parser_parsePackage(parser, node);
        lexeme = parser_peek(parser);

        uint8_t hasAlias = 0;
        char* alias = NULL;
        // is `as` present?
        if(lexeme.type == TOK_TYPE_CONVERSION) {
            hasAlias = 1;
            ACCEPT;
            lexeme = parser_peek(parser);
            ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
            alias = strdup(lexeme.string);
            ACCEPT;
        }
        else {
            parser_reject(parser);
        }
        ImportStmt* import = ast_makeImportStmt(source, target, hasAlias, alias);
        vec_push(&node->programNode->importStatements, import);
        lexeme = parser_peek(parser);

        // here we might find a comma. if we do, we accept it and keep looping
        can_loop = lexeme.type == TOK_COMMA;
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
    Lexeme lexeme = parser_peek(parser);
    // is `as` present?
    if(lexeme.type == TOK_TYPE_CONVERSION) {
        hasAlias = 1;
        ACCEPT;
        lexeme = parser_peek(parser);
        ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        alias = strdup(lexeme.string);
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
    Lexeme lexeme = parser_peek(parser);
    ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier/package` expected but %s was found.", EXPAND_LEXEME);
    vec_push(&package->ids, strdup(lexeme.string));

    ACCEPT;

    lexeme = parser_peek(parser);
    uint8_t can_go = lexeme.type == TOK_DOT;
    while(can_go) {
        lexeme = parser_peek(parser);
        ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier/package` expected but %s was found.", EXPAND_LEXEME);
        vec_push(&package->ids, strdup(lexeme.string));
        ACCEPT;

        lexeme = parser_peek(parser);
        can_go = lexeme.type == TOK_DOT;
    }

    parser_reject(parser);

    return package;
}

#undef CURRENT
#undef ACCEPT
#undef TTTS