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

ASTNode* parser_parse(Parser* parser) {
    ASTNode* node = ast_makeProgramNode();
    parser_parseProgram(parser, node);

    return node;
}

void parser_parseProgram(Parser* parser, ASTNode* node) {
    Lexeme lexeme = parser_peek(parser);

    uint8_t can_loop = lexeme.type == TOK_FROM || lexeme.type == TOK_IMPORT;

    while(can_loop) {
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
    };

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
    printf("%s\n", ast_stringifyType(type_def));

    map_set(&node->scope.dataTypes, type->name, type);
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

/*
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
DataType* parser_parseTypePrimary(Parser* parser, ASTNode* node) {
    // parse enum if current keyword is enum
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_ENUM) {
        return parser_parseTypeEnum(parser, node);
    }
    if(lexeme.type == TOK_STRUCT){
        return parser_parseTypeStruct(parser, node);
    }
    if(lexeme.type == TOK_VARIANT) {
        return parser_parseTypeVariant(parser, node);
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

        // a generic list might follow
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_LESS) {
            type->isGeneric = 1;
            // here, each generic type can be any type, so we recursively parse type
            // format <type> ("," <type>)* ">"
            ACCEPT;
            lexeme = parser_peek(parser);
            uint8_t can_loop = lexeme.type != TOK_GREATER;
            while(can_loop) {
                // parse type
                DataType* genericType = parser_parseTypeUnion(parser, node);
                // add to generic list
                vec_push(&type->concreteGenerics, genericType);
                // check if we have a comma
                lexeme = parser_peek(parser);
                if(lexeme.type == TOK_COMMA) {
                    ACCEPT;
                }
                else {
                    // if no comma, we assert ">"
                    ASSERT(lexeme.type == TOK_GREATER,
                           "Line: %"PRIu16", Col: %"PRIu16" `>` expected but %s was found.",
                           EXPAND_LEXEME);
                    ACCEPT;
                    can_loop = 0;
                }
            }
        }
        else {
            parser_reject(parser);
        }

        return type;
    }
}

// variant type ::= "variant" "{" <variant_decl> (","? <variant_decl>)* "}"
DataType* parser_parseTypeVariant(Parser* parser, ASTNode* node) {
    //create base type
    DataType * variantType = ast_type_makeType();
    variantType->kind = DT_VARIANT;
    variantType->variantType = ast_type_makeVariant();
    // accept variant
    ACCEPT;
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    // make sure we have an ID next
    CURRENT;

    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    while(can_loop) {
        // we get the name of the variant
        // assert we have an ID
        ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);

        char* variantName = lexeme.string;
        // make sure the variant doesn't have constructor with same name, by checking variantType->variantType->constructors
        // using map_get
        ASSERT(map_get(&variantType->variantType->constructors, variantName) == NULL,
               "Line: %"PRIu16", Col: %"PRIu16" near `%s` variant constructor with name %s already exists.", EXPAND_LEXEME);

        // we create a new VariantConstructor
        VariantConstructor* variantConstructor = ast_type_makeVariantConstructor();
        variantConstructor->name = strdup(variantName);
        // accept the name
        ACCEPT;

        CURRENT;
        // check if we have parenthesis
        if(lexeme.type == TOK_LPAREN) {
            // we parse the arguments
            // format: "(" <id>":" <type> (","? <id> ":"<type>)* ")"
            ACCEPT;
            CURRENT;
            // prepare to loop
            uint8_t can_loop_2 = lexeme.type == TOK_IDENTIFIER;
            while(can_loop_2) {
                // assert identifier
                ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" identifier expected but %s was found.", EXPAND_LEXEME);

                // we get the name of the argument
                char* argName = lexeme.string;

                // accept the name
                ACCEPT;
                CURRENT;

                // make sure we have a colon
                ASSERT(lexeme.type == TOK_COLON, "Line: %"PRIu16", Col: %"PRIu16" `:` expected but %s was found.", EXPAND_LEXEME);
                ACCEPT;
                CURRENT;
                // we parse the type
                DataType* argType = parser_parseTypeUnion(parser, node);
                // we create a new argument
                VariantConstructorArgument * arg = ast_type_makeVariantConstructorArgument();
                arg->name = strdup(argName);
                arg->type = argType;
                // add to argument list

                // make sure arg name doesn't already exist
                ASSERT(map_get(&variantConstructor->args, argName) == NULL, "Line: %"PRIu16", Col: %"PRIu16" near %s, argument name `%s` already exists.", EXPAND_LEXEME, argName);
                vec_push(&variantConstructor->argNames, argName);
                map_set(&variantConstructor->args, argName, arg);

                CURRENT;
                // check if we have a comma

                if(lexeme.type == TOK_COMMA) {
                    ACCEPT;
                    CURRENT;
                }
                else if(lexeme.type == TOK_RPAREN) {
                    can_loop_2 = 0;
                    ACCEPT;
                }
                else {
                    // throw error, we need either a comma or a ")"
                    ASSERT(0, "Line: %"PRIu16", Col: %"PRIu16" near %s, `,` or `)` expected but %s was found.", EXPAND_LEXEME);
                }
            }
        }
        // add constructor to map and its name to the vector
        map_set(&variantType->variantType->constructors, variantName, variantConstructor);
        vec_push(&variantType->variantType->constructorNames, variantName);
        // skip comma if any
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
            CURRENT;
        }

        if(lexeme.type == TOK_RBRACE) {
            can_loop = 0;
        }

        //

    }

    return variantType;
}

// struct_type ::= "struct" "{" <struct_decl> ("," <struct_decl>)* "}"
DataType* parser_parseTypeStruct(Parser* parser, ASTNode* node) {
    DataType * structType = ast_type_makeType();
    structType->kind = DT_STRUCT;
    structType->structType = ast_type_makeStruct();

    // currently at struct
    // we skip struct and make sure we have { after
    ACCEPT;
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;
    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    while(can_loop) {
        // we parse in the form of identifier : type
        // create struct attribute
        StructAttribute* attr = ast_type_makeStructAttribute();
        // parse identifier
        ASSERT(lexeme.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        attr->name = strdup(lexeme.string);
        ACCEPT;
        CURRENT;
        // parse :

        ASSERT(lexeme.type == TOK_COLON,
               "Line: %"PRIu16", Col: %"PRIu16" `:` expected but %s was found.", EXPAND_LEXEME);
        ACCEPT;

        // parse type
        attr->type = parser_parseTypeUnion(parser, node);

        // add to struct
        // make sure type doesn't exist first
        ASSERT(map_get(&structType->structType->attributes, attr->name) == NULL,
               "Line: %"PRIu16", Col: %"PRIu16" near %s, attribute `%s` already exists in struct.", EXPAND_LEXEME, attr->name);

        vec_push(&structType->structType->attributeNames, attr->name);
        map_set(&structType->structType->attributes, attr->name, attr);


        // check if we reached the end, means we do not have an ID anymore
        CURRENT;

        // ignore comma if exists
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
            CURRENT;
        }

        if(lexeme.type == TOK_RBRACE) {
            can_loop = 0;
        }
    }
    ACCEPT;
    return structType;
}

// enum_type ::= "enum" "{" <enum_decl> ("," <enum_decl>)* "}"
DataType* parser_parseTypeEnum(Parser* parser, ASTNode* node) {
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