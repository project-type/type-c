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
    }

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

    lexeme = parser_peek(parser);

    if(lexeme.type == TOK_LESS) {
        // GENERICS!
        type->hasGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        ASSERT(lexeme.type == TOK_IDENTIFIER,
               "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);

        while(can_loop) {
            // init generic param
            GenericParam * genericParam = ast_make_genericParam();
            genericParam->isGeneric = 1;

            // in a type declaration, all given templates in the referee are generic and not concrete.
            ASSERT(lexeme.type == TOK_IDENTIFIER,
                   "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.",
                   EXPAND_LEXEME);

            // get generic name
            genericParam->name = strdup(lexeme.string);
            ACCEPT;
            CURRENT;
            // check if have ":"
            if(lexeme.type == TOK_COLON) {
                ACCEPT;
                CURRENT;
                // get generic type
                genericParam->constraint = parser_parseTypeUnion(parser, node, NULL);
                CURRENT;
            }
            else {
                // if not, set to any
                genericParam->constraint = NULL;
            }

            // add generic param
            vec_push(&type->genericParams, genericParam);
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
    DataType* type_def = parser_parseTypeUnion(parser, node, type);
    type->refType = ast_type_makeReference();
    type->refType->ref = type_def;
    printf("%s\n", ast_stringifyType(type_def));

    map_set(&node->scope.dataTypes, type->name, type);
}

// <union_type> ::= <intersection_type> ( "|" <union_type> )*
DataType* parser_parseTypeUnion(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // must parse union type
    DataType* type = parser_parseTypeIntersection(parser, node, parentReferee);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_OR) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeUnion(parser, node, parentReferee);
        // create intersection type
        UnionType * unions = ast_type_makeUnion();
        unions->left = type;
        unions->right = type2;

        // create new datatype to hold joints
        DataType* newType = ast_type_makeType();
        newType->kind = DT_TYPE_UNION;
        newType->unionType = unions;
        type = newType;
    }
    // TODO: make sure that unions lhs and rhs match
    // TODO: make sure that nullable types are valid (memory references)

    parser_reject(parser);
    return type;
}

// <intersection_type> ::= <group_type> ( "&" <group_type> )*
DataType* parser_parseTypeIntersection(Parser* parser, ASTNode* node, DataType* parentReferee){
    // must parse group type
    DataType* type = parser_parseTypeArray(parser, node, parentReferee);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_AND) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeIntersection(parser, node, parentReferee);
        // create intersection type
        JoinType * join = ast_type_makeJoin();
        join->left = type;
        join->left = type2;

        // create new datatype to hold joints
        DataType* newType = ast_type_makeType();
        newType->kind = DT_TYPE_JOIN;
        newType->joinType = join;
        return newType;
    }
    parser_reject(parser);
    return type;
}

// <group_type> ::= <primary_type>  | "(" <union_type> ")" "?"?
DataType* parser_parseTypeGroup(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // check if we have group first
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_LPAREN) {
        // we have a group
        ACCEPT;
        DataType* type = parser_parseTypeUnion(parser, node, parentReferee);
        lexeme = parser_peek(parser);
        ASSERT(lexeme.type == TOK_RPAREN, "Line: %"PRIu16", Col: %"PRIu16" `)` expected but %s was found.", EXPAND_LEXEME);
        ACCEPT;

        // check if we have optional
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_NULLABLE) {
            // we have an optional
            ACCEPT;
            type->isNullable = 1;

        }
        else {
            parser_reject(parser);
        }
        return type;
    }
    parser_reject(parser);
    DataType* type = parser_parseTypePrimary(parser, node, parentReferee);
    return type;
}


// <array_type> ::= <primary_type> "[" "]" | <primary_type> "[" <integer_literal> "]"
DataType * parser_parseTypeArray(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // must parse primary type first
    DataType* primary = parser_parseTypeGroup(parser, node, parentReferee);
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
<primary_type> ::= <interface_type> "?"?
                 | <enum_type> "?"?
                 | <struct_type> "?"?
                 | <data_type> "?"?
                 | <function_type> "?"?
                 | <basic_type> "?"?
                 | <ptr_type> "?"?
                 | <class_type> "?"?
                 | <reference_type> "?"?
 */
DataType* parser_parseTypePrimary(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // parse enum if current keyword is enum
    DataType * type = NULL;
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_ENUM) {
        type = parser_parseTypeEnum(parser, node);
    }
    else if(lexeme.type == TOK_STRUCT){
        type = parser_parseTypeStruct(parser, node, parentReferee);
    }
    else if(lexeme.type == TOK_VARIANT) {
        type = parser_parseTypeVariant(parser, node, parentReferee);
    }
    // check if current keyword is a basic type
    else if((lexeme.type >= TOK_I8) && (lexeme.type <= TOK_CHAR)) {
        // create new type assign basic to it
        DataType* basicType = ast_type_makeType();
        basicType->kind = lexeme.type - TOK_I8;
        ACCEPT;
        type = basicType;
    }
    else if (lexeme.type == TOK_INTERFACE){
        type = parser_parseTypeInterface(parser, node, parentReferee);
    }
    else if (lexeme.type == TOK_FN) {
        type = parser_parseTypeFn(parser, node, parentReferee);
    }
    else if (lexeme.type == TOK_PTR) {
        type = parser_parseTypePtr(parser, node, parentReferee);
    }
    // check if we have an ID, we parse package then
    else if(lexeme.type == TOK_IDENTIFIER) {
        DataType* refType = parser_parseTypeRef(parser, node, parentReferee);

        type = refType;
    }
    // check if we have an optional type
    CURRENT;
    if(lexeme.type == TOK_NULLABLE) {
        type->isNullable = 1;
        ACCEPT;
    }
    else
        parser_reject(parser);

    return type;
}

DataType* parser_parseTypeRef(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // we create a reference refType
    DataType* refType = ast_type_makeType();
    refType->kind = DT_REFERENCE;
    refType->refType = ast_type_makeReference();
    // rollback
    parser_reject(parser);
    refType->refType->pkg = parser_parsePackage(parser, node);

    // a generic list might follow
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_LESS) {
        refType->hasGeneric = 1;
        // here, each generic refType can be any refType, so we recursively parse refType
        // format <refType> ("," <refType>)* ">"
        ACCEPT;
        lexeme = parser_peek(parser);
        uint8_t can_loop = lexeme.type != TOK_GREATER;
        while(can_loop) {
            // parse refType
            DataType* genericType = parser_parseTypeUnion(parser, node, parentReferee);
            GenericParam* gparam = ast_make_genericParam();
            gparam->isGeneric = 0;

            if(parentReferee != NULL) {
                // check if the param is a reference
                if (genericType->kind == DT_REFERENCE) {
                    // check if it has a single name (not entire package a.b.c)
                    if (genericType->refType->pkg->ids.length == 1) {
                        // check if it exists on parent
                        char *gtname = genericType->refType->pkg->ids.data[0];
                        uint32_t i;
                        GenericParam *parentGenericType;
                        vec_foreach(&parentReferee->genericParams, parentGenericType, i) {
                            if (parentGenericType->isGeneric == 1) {
                                if (strcmp(parentGenericType->name, gtname) == 0) {
                                    gparam->isGeneric = 1;
                                    gparam->name = strdup(gtname);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if(!gparam->isGeneric) {
                gparam->type = genericType;
            }

            //add to generic list
            vec_push(&refType->genericParams, gparam);
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

    return refType;
}

// interface_tupe ::= "interface" "{" <interface_decl> (","? <interface_decl>)* "}"
DataType* parser_parseTypeInterface(Parser* parser, ASTNode* node, DataType* parentReferee){
    // create base type
    DataType* interfaceType = ast_type_makeType();
    interfaceType->kind = DT_INTERFACE;
    interfaceType->interfaceType = ast_type_makeInterface();
    // accept interface
    ACCEPT;
    Lexeme CURRENT;

    // if we have "(", means interface extends other interfaces
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;
        parser_parseExtends(parser, node, parentReferee, &interfaceType->interfaceType->extends);
        CURRENT;
    }
    else {
        //parser_reject(parser);
    }


    ASSERT(lexeme.type == TOK_LBRACE, "Line: %"PRIu16", Col: %"PRIu16" `{` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;
    // now we expect an interface method
    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_FN;
    while(can_loop) {
        // parse interface method
        // we assert "fn"
        ASSERT(lexeme.type == TOK_FN,
               "Line: %"PRIu16", Col: %"PRIu16" `fn` expected but %s was found.",
               EXPAND_LEXEME);
        // reject it
        parser_reject(parser);
        FnHeader * header = parser_parseFnHeader(parser, node);
        // make sure fn doesn't already exist
        ASSERT(map_get(&interfaceType->interfaceType->methods, header->name) == NULL,
               "Line: %"PRIu16", Col: %"PRIu16" near %s, method `%s` already exists in interface.",
               EXPAND_LEXEME, header->name);

        // else we add it
        map_set(&interfaceType->interfaceType->methods, header->name, header);
        vec_push(&interfaceType->interfaceType->methodNames, header->name);
        // skip "," if any
        CURRENT;
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
            CURRENT;
        }

        // check if we reached "}"
        if(lexeme.type == TOK_RBRACE) {
            ACCEPT;
            can_loop = 0;
        }
    }

    return interfaceType;
}

// variant_type ::= "variant" "{" <variant_decl> (","? <variant_decl>)* "}"
DataType* parser_parseTypeVariant(Parser* parser, ASTNode* node, DataType* parentReferee) {
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
                DataType* argType = parser_parseTypeUnion(parser, node, parentReferee);
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
            ACCEPT;
        }
    }

    return variantType;
}

// struct_type ::= "struct" "{" <struct_decl> ("," <struct_decl>)* "}"
DataType* parser_parseTypeStruct(Parser* parser, ASTNode* node, DataType* parentReferee) {
    DataType * structType = ast_type_makeType();
    structType->kind = DT_STRUCT;
    structType->structType = ast_type_makeStruct();

    // currently at struct
    // we skip struct and make sure we have { after
    ACCEPT;
    Lexeme CURRENT;
    // if we have "(", means interface extends other interfaces
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;
        parser_parseExtends(parser, node, parentReferee, &structType->structType->extends);
        CURRENT;
    }
    else {
        //parser_reject(parser);
    }
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
        attr->type = parser_parseTypeUnion(parser, node, parentReferee);

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

// parses extends list, must start from first symbol after "("
void parser_parseExtends(Parser* parser, ASTNode* node, DataType* parentReferee, dtype_vec_t* extends){
    Lexeme lexeme = parser_peek(parser);
    uint8_t can_loop = lexeme.type != TOK_RPAREN;
    while(can_loop) {
        // we don't look out for id because it might be an anonymous type
        //ASSERT(lexeme.type == TOK_IDENTIFIER,
        //       "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.",
        //       EXPAND_LEXEME);
        // parse type
        parser_reject(parser);
        DataType* interfaceParentType = parser_parseTypePrimary(parser, node, parentReferee);
        // add to extends
        vec_push(extends, interfaceParentType);

        // check if we have a comma
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
        }
        else {
            // if no comma, we assert ")"
            ASSERT(lexeme.type == TOK_RPAREN,
                   "Line: %"PRIu16", Col: %"PRIu16" `)` expected but %s was found.",
                   EXPAND_LEXEME);
            ACCEPT;
            can_loop = 0;
        }
    }
}
// "fn" "(" <param_list>? ")" ("->" <type>)? <block>
DataType* parser_parseTypeFn(Parser* parser, ASTNode* node, DataType* parentReferee) {
    // current position: fn
    ACCEPT;
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_LPAREN, "Line: %"PRIu16", Col: %"PRIu16" `(` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;

    // create function type
    DataType* fnType = ast_type_makeType();
    fnType->kind = DT_FN;
    fnType->fnType = ast_type_makeFn();
    // parse parameters
    parser_parseFnDefArguments(parser, node, parentReferee, &fnType->fnType->args, &fnType->fnType->argNames);
    // check if we have `->`
    lexeme = parser_peek(parser);
    if(lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        // parse return type
        fnType->fnType->returnType = parser_parseTypePrimary(parser, node, fnType);
        // assert return type is not null
        ASSERT(fnType->fnType->returnType != NULL, "Line: %"PRIu16", Col: %"PRIu16" near %s, return type null detected, this is a parser issue.", EXPAND_LEXEME);
    }
    else {
        parser_reject(parser);
    }

    return fnType;
}

// "ptr" "<" <type> ">"
DataType* parser_parseTypePtr(Parser* parser, ASTNode* node, DataType* parentReferee){
    // build type
    DataType* ptrType = ast_type_makeType();
    ptrType->kind = DT_PTR;
    ptrType->ptrType = ast_type_makePtr();
    // currently at ptr
    ACCEPT;
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_LESS, "Line: %"PRIu16", Col: %"PRIu16" `<` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    // parse type
    ptrType->ptrType->target = parser_parseTypePrimary(parser, node, parentReferee);
    // assert type is not null
    ASSERT(ptrType->ptrType->target != NULL, "Line: %"PRIu16", Col: %"PRIu16" near %s, type null detected, this is a parser issue.", EXPAND_LEXEME);
    // assert we have a ">"
    CURRENT;
    ASSERT(lexeme.type == TOK_GREATER, "Line: %"PRIu16", Col: %"PRIu16" `>` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    return ptrType;
}

// starts from the first argument
void parser_parseFnDefArguments(Parser* parser, ASTNode* node, DataType* parentType, fnargument_map_t* args, vec_str_t* argsNames){
    Lexeme CURRENT;
    uint8_t loop = lexeme.type != TOK_RPAREN;
    while(loop) {
        parser_reject(parser);
        // build fnarg
        FnArgument* fnarg = ast_type_makeFnArgument();
        // check if we have `mut` or id
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_MUT) {
            fnarg->isMutable = 1;
            ACCEPT;
            CURRENT;
        }
        // assert an id
        ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
        fnarg->name = strdup(lexeme.string);
        ACCEPT;
        // assert ":"
        CURRENT;
        ASSERT(lexeme.type == TOK_COLON, "Line: %"PRIu16", Col: %"PRIu16" `:` expected but %s was found.", EXPAND_LEXEME);
        ACCEPT;
        // parse type
        fnarg->type = parser_parseTypePrimary(parser, node, parentType);
        // assert type is not null
        ASSERT(fnarg->type != NULL, "Line: %"PRIu16", Col: %"PRIu16" `type` near %s, generated a NULL type. This is a parser issue.", EXPAND_LEXEME);
        // add to args
        map_set(args, fnarg->name, fnarg);
        vec_push(argsNames, fnarg->name);
        // check if we have a comma
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
        }
        else {
            // if no comma, we assert ")"
            ASSERT(lexeme.type == TOK_RPAREN,
                   "Line: %"PRIu16", Col: %"PRIu16" `)` expected but %s was found.",
                   EXPAND_LEXEME);
            ACCEPT;
            loop = 0;
        }
    }
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

// parses fn header for interfaces, interfaces cannot have mut arguments, they are not allowed
// to mutate arguments given to them.
FnHeader* parser_parseFnHeader(Parser* parser, ASTNode* node) {
    // build header struct
    FnHeader* header = ast_makeFnHeader();
    header->type = ast_type_makeFn();
    // assert we are at "fn"
    Lexeme CURRENT;
    ASSERT(lexeme.type == TOK_FN, "Line: %"PRIu16", Col: %"PRIu16" `fn` expected but %s was found.", EXPAND_LEXEME);
    ACCEPT;
    CURRENT;
    // assert we got a name
    ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected but %s was found.", EXPAND_LEXEME);
    header->name = strdup(lexeme.string);
    // accept name
    ACCEPT;

    // if we have "<" its a generic
    CURRENT;
    if(lexeme.type == TOK_LESS) {
        header->isGeneric = 1;
        ACCEPT;
        // get current lexeme
        CURRENT;

        // prepare to loop
        uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
        uint32_t idx = 0;
        while(can_loop) {
            // assert its an ID
            ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected in function header but %s was found.", EXPAND_LEXEME);

            // add it to our generic list
            // make sure we do not have duplicates by getting the element of the map with the name and asserting it is null
            ASSERT(map_get(&header->generics, lexeme.string) == NULL, "Line: %"PRIu16", Col: %"PRIu16" generic name `%s` already defined.", EXPAND_LEXEME);

            vec_push(&header->genericNames, strdup(lexeme.string));
            map_set(&header->generics, lexeme.string, idx++);

            // get current lexeme
            CURRENT;
            // assert we got a name

            // if we have a ">" we are done
            if(lexeme.type == TOK_GREATER) {
                can_loop = 0;
                ACCEPT;
            }
            else {
                ASSERT(lexeme.type == TOK_COMMA, "Line: %"PRIu16", Col: %"PRIu16" `,` or `>` expected in generic list but %s was found.", EXPAND_LEXEME);
                ACCEPT;
                CURRENT;
            }
        }
    }
    // assert we have "("
    ASSERT(lexeme.type == TOK_LPAREN, "Line: %"PRIu16", Col: %"PRIu16" `(` expected after function name but %s was found.", EXPAND_LEXEME);
    ACCEPT;

    // we are going to parse the arguments
    CURRENT;
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    // create fnHeader object
    while(can_loop) {
        // ASSERT ID
        ASSERT(lexeme.type == TOK_IDENTIFIER, "Line: %"PRIu16", Col: %"PRIu16" `identifier` expected for arg declaration but %s was found.", EXPAND_LEXEME);
        // accept ID
        char* name = strdup(lexeme.string);
        ACCEPT;
        CURRENT;
        // assert ":"
        ASSERT(lexeme.type == TOK_COLON, "Line: %"PRIu16", Col: %"PRIu16" `:` expected after arg name but %s was found.", EXPAND_LEXEME);
        ACCEPT;
        CURRENT;
        // assert type
        DataType* type = parser_parseTypeUnion(parser, node, NULL);

        // make sure arg doesnt already exist
        ASSERT(map_get(&header->type->args, name) == NULL, "Line: %"PRIu16", Col: %"PRIu16" argument name `%s` already exists.", EXPAND_LEXEME);

        // make FnArg
        FnArgument * arg = ast_type_makeFnArgument(name, type);
        arg->type = type;
        arg->isMutable = 0;
        arg->name = name;

        // add arg to header
        vec_push(&header->type->argNames, name);
        map_set(&header->type->args, name, arg);

        // check if we reached ")"
        CURRENT;
        if(lexeme.type == TOK_RPAREN) {
            can_loop = 0;
            ACCEPT;
        }
        else {
            ASSERT(lexeme.type == TOK_COMMA, "Line: %"PRIu16", Col: %"PRIu16" `,` or `)` expected after arg declaration but %s was found.", EXPAND_LEXEME);
            ACCEPT;
            CURRENT;
        }
    }

    // do we have a return type?
    CURRENT;
    if(lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        header->type->returnType = parser_parseTypeUnion(parser, node, NULL);
    }
    else{
        parser_reject(parser);
    }

    return header;

}

#undef CURRENT
#undef ACCEPT
#undef TTTS