//
// Created by praisethemoon on 28.04.23.
//

#include "error.h"
#include <assert.h>
#include <stdlib.h>
#include <printf.h>
#include "../utils/vec.h"
#include "parser.h"
#include "parser_resolve.h"
#include "ast.h"
#include "tokens.h"
#include "ast_json.h"
#include "parser_utils.h"
#include "scope.h"
#include "type_checker.h"
#include "type_inference.h"

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
    ASTProgramNode * node = ast_makeProgramNode();
    parser->programNode = node;
    parser_parseProgram(parser, node);
    ti_runProgram(parser, node);

    return;
}

void parser_parseProgram(Parser* parser, ASTProgramNode * node) {
    Lexeme lexeme = parser_peek(parser);

    uint8_t can_loop = lexeme.type == TOK_FROM || lexeme.type == TOK_IMPORT;

    while(can_loop) {
        if(lexeme.type == TOK_FROM) {
            ACCEPT;
            parser_parseFromStmt(parser, node->scope);
        }
        if(lexeme.type == TOK_IMPORT) {
            ACCEPT;
            parser_parseImportStmt(parser, node->scope);
        }
        char* imports = ast_json_serializeImports(node);
        printf("%s\n", imports);
        lexeme = parser_peek(parser);
        can_loop = lexeme.type == TOK_FROM || lexeme.type == TOK_IMPORT;
    }

    // TODO: Resolve Imports and add them to symbol table
    // we no longer expect import or from after this

    can_loop = 1; //lexeme.type == TOK_TYPE;
    while (can_loop) {
        switch(lexeme.type) {
            case TOK_EXTERN: {
                parser_reject(parser);
                ExternDecl *externDecl = parser_parseExternDecl(parser, node->scope);
                // add externDecl to symbol table
                ScopeRegResult res = scope_registerFFI(node->scope, externDecl);
                if (res == SRRT_TOKEN_ALREADY_REGISTERED) {
                    PARSER_ASSERT(0, "ExternDecl %s already registered", externDecl->name);
                }

                //char* strDecl = ast_json_serializeExternDecl(externDecl);
                //printf("%s\n", strDecl);
                CURRENT;
                break;
            }
            case TOK_TYPE: {
                parser_parseTypeDecl(parser, node->scope);
                CURRENT;
                break;
            }
            case TOK_EOF: {
                printf("EOF reached, Parsing done.\n");
                return;
            }
            default:
            {
                parser_reject(parser);

                Statement *stmt = parser_parseStmt(parser, node->scope);
                CURRENT;
                if (stmt == NULL) {
                    PARSER_ASSERT(0, "Invalid token %s", token_type_to_string(lexeme.type));
                }

                vec_push(&node->stmts, stmt);
                //printf("%s\n", ast_json_serializeStatement(stmt));

                /*
                Expr* expr = parser_parseExpr(parser, node->scope);

                printf("%s\n", ast_json_serializeExpr(expr));
                CURRENT;
                 */
                break;
            }
        }
    }
    printf("Gracefully exiting\n");
}

ExternDecl* parser_parseExternDecl(Parser* parser, ASTScope* currentScope){
    ExternDecl* externDecl = ast_externdecl_make();

    Lexeme lexeme = parser_peek(parser);
    // assert extern
    PARSER_ASSERT(lexeme.type == TOK_EXTERN, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // assert string token
    lexeme = parser_peek(parser);
    PARSER_ASSERT(lexeme.type == TOK_STRING_VAL, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    // assert value is "C"
    PARSER_ASSERT(strcmp(lexeme.string, "\"C\"") == 0, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // assert identifier
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    externDecl->name = strdup(lexeme.string);
    ACCEPT;
    // assert {
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    // now we expect an interface method
    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_FN;
    while(can_loop) {
        // parse interface method
        // we assert "fn"
        PARSER_ASSERT(lexeme.type == TOK_FN, "`fn` expected but %s was found.", token_type_to_string(lexeme.type));
        // reject it
        parser_reject(parser);
        FnHeader * header = parser_parseFnHeader(parser, currentScope);
        // make sure fn doesn't already exist
        PARSER_ASSERT(scope_ffi_addMethod(externDecl, header),
               "method name `%s` already exists in ffi.", header->name);

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
    return externDecl;
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
void parser_parseTypeDecl(Parser* parser, ASTScope* currentScope) {
    DataType * type = ast_type_makeType(currentScope, parser->stack.data[0], DT_REFERENCE);
    ACCEPT;
    Lexeme lexeme = parser_peek(parser);
    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    type->name = strdup(lexeme.string);
    ACCEPT;

    lexeme = parser_peek(parser);

    if(lexeme.type == TOK_LESS) {
        // GENERICS!
        type->isGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
               "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        uint32_t idx = 0;
        while(can_loop) {
            // init generic param
            GenericParam * genericParam = ast_make_genericParam();

            // in a type declaration, all given templates in the referee are generic and not concrete.
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                   "identifier expected but %s was found.", token_type_to_string(lexeme.type));

            // get generic name
            genericParam->name = strdup(lexeme.string);
            PARSER_ASSERT(scope_dtype_addGeneric(type, genericParam), "generic param `%s` already exists in type `%s`.", genericParam->name, type->name);
            ACCEPT;
            CURRENT;
            // check if have ":"
            if(lexeme.type == TOK_COLON) {
                ACCEPT;
                CURRENT;
                // get generic type
                genericParam->constraint = parser_parseTypeUnion(parser, NULL, currentScope);
                CURRENT;
            }
            else {
                // if not, set to any
                genericParam->constraint = NULL;
            }

            if(lexeme.type == TOK_GREATER){
                can_loop = 0;
                ACCEPT;
                CURRENT;
            }

            else {
                PARSER_ASSERT(lexeme.type == TOK_COMMA,
                       "`,` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                CURRENT;
            }
        }
    }

    PARSER_ASSERT(lexeme.type == TOK_EQUAL, "`=` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    //lexeme = parser_peek(parser);
    DataType* type_def = parser_parseTypeUnion(parser, type, currentScope);
    type->refType = ast_type_makeReference();
    type->refType->ref = type_def;
    //printf("%s\n", ast_stringifyType(type_def));
    printf("%s\n", ast_json_serializeDataType(type_def));

    PARSER_ASSERT(scope_registerType(currentScope, type), "type `%s` already exists.", type->name);
}

// <union_type> ::= <intersection_type> ( "|" <union_type> )*
DataType* parser_parseTypeUnion(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // must parse union type
    DataType* type = parser_parseTypeIntersection(parser, parentReferee, currentScope);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_OR) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeUnion(parser, parentReferee, currentScope);
        uint8_t canJoin = tc_check_canJoinOrUnion(parser, currentScope, type, type2);
        PARSER_ASSERT(canJoin, "Cannot create union data types of different categories `%s` and `%s`", dataTypeKindToString(type), dataTypeKindToString(type2));
        char* unionResult = tc_check_canUnion(parser, currentScope,type, type2);
        PARSER_ASSERT(unionResult == NULL, "Cannot create union of these types, duplicate field `%s` found. ", unionResult);

        // create intersection type
        UnionType * unions = ast_type_makeUnion();
        unions->left = type;
        unions->right = type2;


        // create new datatype to hold joints
        DataType* newType = ast_type_makeType(currentScope, parser->stack.data[0], DT_TYPE_UNION);
        newType->unionType = unions;
        type = newType;
    }

    parser_reject(parser);
    return type;
}

// <intersection_type> ::= <group_type> ( "&" <group_type> )*
DataType* parser_parseTypeIntersection(Parser* parser, DataType* parentReferee, ASTScope* currentScope){
    // must parse group type
    DataType* type = parser_parseTypeArray(parser, parentReferee, currentScope);
    // check if we have intersection
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_BITWISE_AND) {
        // we have an intersection
        ACCEPT;
        DataType* type2 = parser_parseTypeIntersection(parser, parentReferee, currentScope);
        uint8_t canJoin = tc_check_canJoinOrUnion(parser, currentScope,type, type2);
        PARSER_ASSERT(canJoin, "Cannot create join data types of different categories `%s` and `%s`", dataTypeKindToString(type), dataTypeKindToString(type2));
        char* unionResult = tc_check_canJoin(parser, currentScope,type, type2);
        PARSER_ASSERT(unionResult == NULL, "Cannot create join of these types, duplicate field `%s` found. ", unionResult);
        // create intersection type
        JoinType * join = ast_type_makeJoin();
        join->left = type;
        join->right = type2;

        // create new datatype to hold joints
        DataType* newType = ast_type_makeType(currentScope, parser->stack.data[0], DT_TYPE_JOIN);
        newType->joinType = join;
        return newType;
    }
    parser_reject(parser);
    return type;
}

// <group_type> ::= <primary_type>  | "(" <union_type> ")" "?"?
DataType* parser_parseTypeGroup(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // check if we have group first
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_LPAREN) {
        // we have a group
        ACCEPT;
        DataType* type = parser_parseTypeUnion(parser, parentReferee, currentScope);
        lexeme = parser_peek(parser);
        PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
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
    DataType* type = parser_parseTypePrimary(parser, parentReferee, currentScope);
    return type;
}

// <array_type> ::= <primary_type> "[" "]" | <primary_type> "[" <integer_literal> "]"
DataType * parser_parseTypeArray(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // must parse primary type first
    DataType* primary = parser_parseTypeGroup(parser, parentReferee, currentScope);
    Lexeme lexeme = parser_peek(parser);
    DataType * last_type = primary;
    // if next token is "[" then it is an array

    uint8_t can_loop = lexeme.type == TOK_LBRACKET;

    if(!can_loop){
        parser_reject(parser);
        return primary;
    }

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
                PARSER_ASSERT((lexeme.type == TOK_INT) || (lexeme.type == TOK_HEX_INT) ||
                       (lexeme.type == TOK_OCT_INT) || (lexeme.type == TOK_BINARY_INT),
                       "`int` expected but %s was found.", token_type_to_string(lexeme.type));
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
                PARSER_ASSERT(lexeme.type == TOK_RBRACKET,
                       "`]` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
            }

            DataType * retType = ast_type_makeType(currentScope, parser->stack.data[0], DT_ARRAY);
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
DataType* parser_parseTypePrimary(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // parse enum if current keyword is enum
    DataType * type = NULL;
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_ENUM) {
        type = parser_parseTypeEnum(parser, currentScope);
    }
    else if(lexeme.type == TOK_STRUCT){
        type = parser_parseTypeStruct(parser, parentReferee, currentScope);
    }
    else if(lexeme.type == TOK_VARIANT) {
        type = parser_parseTypeVariant(parser, parentReferee, currentScope);
    }
    // check if current keyword is a basic type
    else if((lexeme.type >= TOK_I8) && (lexeme.type <= TOK_CHAR)) {
        if(lexeme.type == TOK_VOID){
            DataTypeKind t = lexeme.type - TOK_I8;
            DataTypeKind t2 = lexeme.type - TOK_I8;
        }
        // create new type assign basic to it
        DataType* basicType = ast_type_makeType(currentScope, parser->stack.data[0], lexeme.type - TOK_I8);
        ACCEPT;
        type = basicType;
    }
    else if (lexeme.type == TOK_CLASS){
        type = parser_parseTypeClass(parser, parentReferee, currentScope);
    }
    else if (lexeme.type == TOK_INTERFACE){
        type = parser_parseTypeInterface(parser, parentReferee, currentScope);
    }
    else if (lexeme.type == TOK_FN) {
        type = parser_parseTypeFn(parser, parentReferee, currentScope);
    }
    else if (lexeme.type == TOK_PTR) {
        type = parser_parseTypePtr(parser, parentReferee, currentScope);
    }
    else if (lexeme.type == TOK_PROCESS) {
        type = parser_parseTypeProcess(parser, parentReferee, currentScope);
    }
    // check if we have an ID, we parse package then
    else if(lexeme.type == TOK_IDENTIFIER) {
        DataType* refType = parser_parseTypeRef(parser, parentReferee, currentScope);

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

DataType* parser_parseTypeRef(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // we create a reference refType
    DataType* refType = ast_type_makeType(currentScope, parser->stack.data[0], DT_REFERENCE);
    refType->refType = ast_type_makeReference();
    // rollback
    parser_reject(parser);
    refType->refType->pkg = parser_parsePackage(parser, currentScope);

    // a generic list might follow
    Lexeme lexeme = parser_peek(parser);
    if(lexeme.type == TOK_LESS) {
        refType->hasGenerics = 1;
        // here, each generic refType can be any refType, so we recursively parse refType
        // format <refType> ("," <refType>)* ">"
        ACCEPT;
        lexeme = parser_peek(parser);
        uint8_t can_loop = lexeme.type != TOK_GREATER;
        while(can_loop) {
            // parse refType
            DataType* genericType = parser_parseTypeUnion(parser, parentReferee, currentScope);
            vec_push(&refType->genericRefs, genericType);

            lexeme = parser_peek(parser);
            if(lexeme.type == TOK_COMMA) {
                ACCEPT;
            }
            else {
                // if no comma, we assert ">"
                PARSER_ASSERT(lexeme.type == TOK_GREATER,
                       "`>` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                can_loop = 0;
            }
        }
    }
    else {
        parser_reject(parser);
    }
    // check if the type is simple id
    if (refType->refType->pkg->ids.length == 1){
        if((parentReferee != NULL) && (strcmp(parentReferee->name, refType->refType->pkg->ids.data[0]) == 0)) {
            PARSER_ASSERT(0, "Type `%s` cannot reference itself.", refType->refType->pkg->ids.data[0]);
        }

        // TODO: change this once we have imports running
        //DataType* t = resolver_resolveType(parser, currentScope, refType->refType->pkg->ids.data[0]);
        //PARSER_ASSERT(t != NULL, "Type `%s` is not defined.", refType->refType->pkg->ids.data[0]);
        //refType->refType->ref = t;

    }

    return refType;
}

// interface_tupe ::= "interface" "{" <interface_decl> (","? <interface_decl>)* "}"
DataType* parser_parseTypeInterface(Parser* parser, DataType* parentReferee, ASTScope* currentScope){
    // create base type
    DataType* interfaceType = ast_type_makeType(currentScope, parser->stack.data[0], DT_INTERFACE);
    interfaceType->interfaceType = ast_type_makeInterface(currentScope);

    // add the parent type to scope if it exists
    if(parentReferee != NULL) {
        scope_registerType(interfaceType->interfaceType->scope, parentReferee);
    }

    // accept interface
    ACCEPT;
    Lexeme CURRENT;

    // if we have "(", means interface extends other interfaces
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;
        parser_parseExtends(parser, parentReferee, &interfaceType->interfaceType->extends, currentScope, DT_INTERFACE);
        CURRENT;
    }
    /*else {
        //parser_reject(parser);
    }*/

    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    // now we expect an interface method
    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_FN;
    if(!can_loop) {
        // ASSERT }
        PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        return interfaceType;
    }
    while(can_loop) {
        // parse interface method
        // we assert "fn"
        PARSER_ASSERT(lexeme.type == TOK_FN,
               "`fn` expected but %s was found.", token_type_to_string(lexeme.type));
        // reject it
        parser_reject(parser);
        FnHeader * header = parser_parseFnHeader(parser, interfaceType->interfaceType->scope);
        char* duplicate = scope_interface_addMethod(parser, interfaceType->interfaceType->scope, interfaceType, header);
        PARSER_ASSERT(duplicate == NULL,
                     "Method `%s` is already defined in interface.",
                     duplicate);
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

DataType* parser_parseTypeClass(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    DataType * classType = ast_type_makeType(currentScope, parser->stack.data[0], DT_CLASS);
    classType->classType = ast_type_makeClass(currentScope, classType);
    // add the parent type to scope if it exists
    if(parentReferee != NULL) {
        scope_registerType(classType->classType->scope, parentReferee);
    }

    // accept class
    ACCEPT;
    Lexeme CURRENT;

    // if we have "(", means interface extends other interfaces
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;
        parser_parseExtends(parser, parentReferee, &classType->classType->extends, currentScope, DT_INTERFACE);
        CURRENT;
    }
    else {

        //parser_reject(parser);
    }


    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;


    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_FN || lexeme.type == TOK_LET;
    if(!can_loop) {
        // ASSERT }
        PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found", token_type_to_string(lexeme.type));
        ACCEPT;
        return classType;
    }

    while(can_loop) {
        // parse interface method
        // we assert "fn"

        PARSER_ASSERT(lexeme.type == TOK_FN || lexeme.type == TOK_LET,
               "`fn` or `let` expected but %s was found", token_type_to_string(lexeme.type));
        // reject it
        if(lexeme.type == TOK_FN){
            parser_reject(parser);
            Statement * stmt = parser_parseStmtFn(parser, classType->classType->scope);
            // assert stmt != NULL
            PARSER_ASSERT(stmt != NULL, "invalid token `%s`", token_type_to_string(lexeme.type));

            FnDeclStatement * fnDecl = stmt->fnDecl;
            // add method name
            ClassMethod * classMethod = ast_type_makeClassMethod();
            classMethod->decl = fnDecl;
            char* duplicated = scope_class_addMethod(parser, classType->classType->scope, classType, classMethod);
            PARSER_ASSERT(duplicated == NULL,
                         "Method `%s` is already defined in class.",
                         duplicated);

        }
        else {
            parser_reject(parser);
            Statement *stmt = parser_parseStmtLet(parser, classType->classType->scope);
            VarDeclStatement* varDecl = stmt->varDecl;
            uint32_t i = 0; LetExprDecl* letDecl;
            vec_foreach(&varDecl->letList, letDecl, i){
                char* duplicated = scope_class_addAttribute(parser, classType->classType->scope, classType, letDecl);
                PARSER_ASSERT(duplicated == NULL,
                                            "Attribute `%s` is already defined in class.",
                                            duplicated)
            }

            free(varDecl);
            free(stmt);
        }
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

    return classType;
}

// variant_type ::= "variant" "{" <variant_decl> (","? <variant_decl>)* "}"
DataType* parser_parseTypeVariant(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    //create base type
    DataType * variantType = ast_type_makeType(currentScope, parser->stack.data[0], DT_VARIANT);
    variantType->variantType = ast_type_makeVariant(currentScope);
    // add the parent type to scope if it exists
    if(parentReferee != NULL) {
        scope_registerType(variantType->variantType->scope, parentReferee);
    }

    // accept variant
    ACCEPT;
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // make sure we have an ID next
    CURRENT;

    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    while(can_loop) {
        // we get the name of the variant
        // assert we have an ID
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found", token_type_to_string(lexeme.type));

        char* variantName = lexeme.string;
        // make sure the variant doesn't have constructor with same name, by checking variantType->variantType->constructors
        // using map_get
        PARSER_ASSERT(map_get(&variantType->variantType->constructors, variantName) == NULL,
               "variant constructor with name %s already exists.", variantName);

        // we create a new VariantConstructor
        VariantConstructor* variantConstructor = ast_type_makeVariantConstructor();
        variantConstructor->name = strdup(variantName);
        // we add the constructor to the variant
        PARSER_ASSERT(scope_variant_addConstructor(variantType->variantType, variantConstructor),
               "variant constructor with name %s already exists.", variantName);
        // accept the name
        ACCEPT;

        CURRENT;
        // assert we have a "("
        PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
        // we parse the arguments
        // format: "(" <id>":" <type> (","? <id> ":"<type>)* ")"
        ACCEPT;
        CURRENT;
        // prepare to loop
        uint8_t can_loop_2 = lexeme.type == TOK_IDENTIFIER;
        while(can_loop_2) {
            // assert identifier
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));

            // we get the name of the argument
            char* argName = lexeme.string;
            VariantConstructorArgument * arg = ast_type_makeVariantConstructorArgument();
            arg->name = strdup(argName);
            PARSER_ASSERT(scope_variantConstructor_addArg(variantConstructor, arg), "Argument name `%s` already exists in variant constructor", argName);

            // accept the name
            ACCEPT;
            CURRENT;

            // make sure we have a colon
            PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;
            CURRENT;
            // we parse the type
            DataType* argType = parser_parseTypeUnion(parser, NULL, variantType->variantType->scope);
            arg->type = argType;

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
                PARSER_ASSERT(0, "`,` or `)` expected but %s was found.", token_type_to_string(lexeme.type));
            }
        }

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
DataType* parser_parseTypeStruct(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    DataType * structType = ast_type_makeType(currentScope, parser->stack.data[0], DT_STRUCT);
    structType->structType = ast_type_makeStruct(currentScope);

    if(parentReferee != NULL) {
        scope_registerType(structType->structType->scope, parentReferee);
    }

    // currently at struct
    // we skip struct and make sure we have { after
    ACCEPT;
    Lexeme CURRENT;
    // if we have "(", means interface extends other interfaces
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;
        parser_parseExtends(parser, parentReferee, &structType->structType->extends, currentScope, DT_STRUCT);
        CURRENT;
    }
    else {
        //parser_reject(parser);
    }
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    // prepare to loop
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    while(can_loop) {
        // we parse in the form of identifier : type
        // create struct attribute
        StructAttribute* attr = ast_type_makeStructAttribute();
        // parse identifier
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
               "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        attr->name = strdup(lexeme.string);
        char* dup = scope_struct_addAttribute(parser, structType->scope, structType, attr);
        PARSER_ASSERT(dup == NULL, "attribute with name %s already exists in struct.", dup)
        ACCEPT;
        CURRENT;
        // parse :

        PARSER_ASSERT(lexeme.type == TOK_COLON,
               "`:` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        // parse type
        attr->type = parser_parseTypeUnion(parser, parentReferee, currentScope);

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
DataType* parser_parseTypeEnum(Parser* parser, ASTScope* currentScope) {
    EnumType* enum_ = ast_type_makeEnum();
    // current position: enum
    ACCEPT;
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;

    uint8_t can_loop = 1;
    uint32_t index = 0;

    while(can_loop) {
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
               "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        char* name = lexeme.string;
        // TODO: make sure index doesn't exeed some limit?

        if (map_get(&enum_->enums, name) != NULL) {
            PARSER_ASSERT(1==0, "enum value duplicated: %s", token_type_to_string(lexeme.type));
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
    PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    DataType * enumType = ast_type_makeType(currentScope, parser->stack.data[0], DT_ENUM);
    enumType->enumType = enum_;
    return enumType;
}

/*
 * "from" <package_id> "import" <package_id> ("as" <id>)? ("," <package_id> ("as" <id>)?)*
 */
void parser_parseFromStmt(Parser* parser, ASTScope* currentScope){
    // from has already been accepted
    // we expect a namespace x.y.z
    PackageID* source = parser_parsePackage(parser, currentScope);
    Lexeme lexeme = parser_peek(parser);
    // next we need an import
    PARSER_ASSERT(lexeme.type == TOK_IMPORT, "`import` expected but %s was found.", token_type_to_string(lexeme.type));

    ACCEPT;

    uint8_t can_loop = 1;
    do {
        PackageID* target = parser_parsePackage(parser, currentScope);
        lexeme = parser_peek(parser);

        uint8_t hasAlias = 0;
        char* alias = NULL;
        // is `as` present?
        if(lexeme.type == TOK_TYPE_CONVERSION) {
            hasAlias = 1;
            ACCEPT;
            lexeme = parser_peek(parser);
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
            alias = strdup(lexeme.string);
            ACCEPT;
        }
        else {
            parser_reject(parser);
        }
        ImportStmt* import = ast_makeImportStmt(source, target, hasAlias, alias);

        PARSER_ASSERT(scope_program_addImport(parser->programNode, import) == 0, "import already exists.");
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
void parser_parseExtends(Parser* parser, DataType* parentReferee, vec_dtype_t* extends, ASTScope* currentScope, DataTypeKind kind){
    Lexeme lexeme = parser_peek(parser);
    uint8_t can_loop = lexeme.type != TOK_RPAREN;
    while(can_loop) {
        // we don't look out for id because it might be an anonymous type
        //PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
        //       "identifier expected but %s was found.",
        //       EXPAND_LEXEME);
        // parse type
        parser_reject(parser);
        DataType* interfaceParentType = parser_parseTypePrimary(parser, parentReferee, currentScope);
        // add to extends
        //vec_push(extends, interfaceParentType);
        PARSER_ASSERT(scope_canExtend(parser, currentScope, interfaceParentType, kind), "Parent category `%s` doesn't match child category.",
                      dataTypeKindToString(interfaceParentType));
        char* pushRes = scope_extends_addParent(parser, currentScope, extends, interfaceParentType);
        PARSER_ASSERT(pushRes == NULL, "Duplicate field `%s` in parent already exists.", pushRes);

        // check if we have a comma
        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
        }
        else {
            // if no comma, we assert ")"
            PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;
            can_loop = 0;
        }
    }
}
// "fn" "(" <param_list>? ")" ("->" <type>)? <block>
DataType* parser_parseTypeFn(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // current position: fn
    ACCEPT;
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;

    // create function type
    DataType* fnType = ast_type_makeType(currentScope, parser->stack.data[0], DT_FN);
    fnType->fnType = ast_type_makeFn();
    // parse parameters
    parser_parseFnDefArguments(parser, parentReferee, fnType->fnType, currentScope);
    // check if we have `->`
    lexeme = parser_peek(parser);
    if(lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        // parse return type
        fnType->fnType->returnType = parser_parseTypeUnion(parser, fnType, currentScope);
        // assert return type is not null
        PARSER_ASSERT(fnType->fnType->returnType != NULL, "Unexpected symbol while parsing data type `%s`.", token_type_to_string(lexeme.type));
    }
    else {
        parser_reject(parser);
    }

    return fnType;
}

// "ptr" "<" <type> ">"
DataType* parser_parseTypePtr(Parser* parser, DataType* parentReferee, ASTScope* currentScope){
    // build type
    DataType* ptrType = ast_type_makeType(currentScope, parser->stack.data[0], DT_PTR);
    ptrType->ptrType = ast_type_makePtr();
    // currently at ptr
    ACCEPT;
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LESS, "`<` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse type
    ptrType->ptrType->target = parser_parseTypeUnion(parser, parentReferee, currentScope);
    // assert type is not null
    PARSER_ASSERT(ptrType->ptrType->target != NULL, "Invalid symbol `%s` while parsing pointer target", token_type_to_string(lexeme.type));
    // assert we have a ">"
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_GREATER, "`>` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    return ptrType;
}

DataType * parser_parseTypeProcess(Parser* parser, DataType* parentReferee, ASTScope* currentScope){
    DataType * processType = ast_type_makeType(currentScope, parser->stack.data[0], DT_PROCESS);
    processType->processType = ast_type_makeProcess();
    ACCEPT;
    // assert we have a "<"
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LESS, "`<` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse type
    processType->processType->inputType = parser_parseTypeUnion(parser, parentReferee, currentScope);
    CURRENT;
    // assert we have a ","
    PARSER_ASSERT(lexeme.type == TOK_COMMA, "`,` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    processType->processType->outputType = parser_parseTypeUnion(parser, parentReferee, currentScope);
    CURRENT;
    // assert we have a ">"
    PARSER_ASSERT(lexeme.type == TOK_GREATER, "`>` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // assert "("
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    uint8_t loop = lexeme.type != TOK_RPAREN;
    while(loop){
        // create argument
        FnArgument* fnarg = ast_type_makeFnArgument();
        // assert ID
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        fnarg->name = strdup(lexeme.string);
        PARSER_ASSERT(scope_process_AddArg(processType->processType, fnarg), "Duplicate argument `%s` in process constructor.", fnarg->name);
        ACCEPT;
        CURRENT;
        // assert ":"
        PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        // parse type
        fnarg->type = parser_parseTypeUnion(parser, parentReferee, currentScope);
        CURRENT;
        // check if we have "," else assert ) and stop
        if(lexeme.type == TOK_COMMA){
            ACCEPT;
            CURRENT;
        }
        else {
            PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
            loop = 0;
        }
        // add argument
    }
    ACCEPT;
    CURRENT;
    // assert "{"
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    parser_reject(parser);
    // parse block
    processType->processType->body = parser_parseStmtBlock(parser, currentScope);
    PARSER_ASSERT(scope_process_hasReceive(processType->processType), "Scope must have only one statement, a function, called `receive`.")
    return processType;
}

// starts from the first argument
void parser_parseFnDefArguments(Parser* parser, DataType* parentType, FnType* fnType, ASTScope* currentScope){
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
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        fnarg->name = strdup(lexeme.string);
        PARSER_ASSERT(scope_fntype_addArg(fnType, fnarg), "Duplicate argument `%s` in function type definition.", fnarg->name);
        ACCEPT;
        // assert ":"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        // parse type
        fnarg->type = parser_parseTypeUnion(parser, parentType, currentScope);
        // assert type is not null
        PARSER_ASSERT(fnarg->type != NULL, "Invalid symbol `%s` while parsing function argument type", token_type_to_string(lexeme.type));

        lexeme = parser_peek(parser);
        if(lexeme.type == TOK_COMMA) {
            ACCEPT;
        }
        else {
            // if no comma, we assert ")"
            PARSER_ASSERT(lexeme.type == TOK_RPAREN,
                   "`)` expected but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;
            loop = 0;
        }
    }
}

/*
 * "import" <package_id> ("as" <id>)?
 */
void parser_parseImportStmt(Parser* parser, ASTScope* currentScope){
    // from has already been accepted
    // we expect a namespace x.y.z
    PackageID* source = parser_parsePackage(parser, currentScope);
    uint8_t hasAlias = 0;
    char* alias = NULL;
    Lexeme lexeme = parser_peek(parser);
    // is `as` present?
    if(lexeme.type == TOK_TYPE_CONVERSION) {
        hasAlias = 1;
        ACCEPT;
        lexeme = parser_peek(parser);
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        alias = strdup(lexeme.string);
        ACCEPT;
    }
    else{
        parser_reject(parser);
    }

    PackageID * empty = ast_makePackageID();
    ImportStmt* import = ast_makeImportStmt(source, empty, hasAlias, alias);

    PARSER_ASSERT(scope_program_addImport(parser->programNode, import), "Import already exists");
}


/** Expressions **/
Expr* parser_parseExpr(Parser* parser, ASTScope* currentScope) {
    return parser_parseLetExpr(parser, currentScope);
}

/*
 * let <id> (":" <type>)? "=" <uhs> "in" <uhs>
 * let "{"<id> (":" <type>)? (<id> (":" <type>)?)*"}" "=" <uhs> "in" <uhs>
 * let "["<id> (":" <type>)? (<id> (":" <type>)?)*"]" "=" <uhs> "in" <uhs>
 */
Expr* parser_parseLetExpr(Parser* parser, ASTScope* currentScope) {
    Lexeme CURRENT;
    if(lexeme.type == TOK_LET)
    {
        Expr *expr = ast_expr_makeExpr(ET_LET, lexeme);
        LetExpr *let = ast_expr_makeLetExpr(currentScope);

        expr->letExpr = let;

        // assert let
        PARSER_ASSERT(lexeme.type == TOK_LET, "`let` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        CURRENT;
        uint8_t loop_over_expr = 1;
        while (loop_over_expr) {
            LetExprDecl *letDecl = ast_expr_makeLetExprDecl();
            char *expect = NULL;

            // check if we have id or { or [
            if (lexeme.type == TOK_LBRACE) {
                expect = "]";
                letDecl->initializerType = LIT_STRUCT_DECONSTRUCTION;
                ACCEPT;
                CURRENT;
            } else if (lexeme.type == TOK_LBRACKET) {
                expect = "]";
                letDecl->initializerType = LIT_ARRAY_DECONSTRUCTION;
                ACCEPT;
                CURRENT;
            } else {
                expect = NULL;
                letDecl->initializerType = LIT_NONE;
            }

            uint8_t loop = 1;
            while (loop) {
                FnArgument *var = ast_type_makeFnArgument();
                // check if we have a mut
                if (lexeme.type == TOK_MUT) {
                    var->isMutable = 1;
                    ACCEPT;
                    CURRENT;
                }
                // assert ID
                PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                       "identifier expected but %s was found.", token_type_to_string(lexeme.type));
                var->name = strdup(lexeme.string);
                ACCEPT;
                CURRENT;
                // assert ":"
                if(lexeme.type == TOK_COLON) {
                    PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    // parse type
                    var->type = parser_parseTypeUnion(parser, NULL, currentScope);
                    // assert type is not null
                    PARSER_ASSERT(var->type != NULL, "Invalid symbol `%s` while parsing variable type", token_type_to_string(lexeme.type));

                    // check if we have a comma
                    CURRENT;
                }
                // add to args
                // add vars to scope
                PARSER_ASSERT(scope_registerVariable(currentScope, var), "Variable `%s` already exists", var->name);

                map_set(&letDecl->variables, var->name, var);
                vec_push(&letDecl->variableNames, var->name);

                if (lexeme.type == TOK_COMMA) {
                    ACCEPT;
                    CURRENT;
                } else {
                    parser_reject(parser);
                    loop = 0;
                }
            }
            // if we expected something earlier, we must find it closed now
            if (expect != NULL) {
                CURRENT;
                // if we expected "]", we must find "]"
                if (expect[0] == '[') {
                    // assert "]"
                    PARSER_ASSERT(lexeme.type == TOK_RBRACKET,
                           "`]` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    CURRENT;
                } else if (expect[0] == '{') {
                    // assert "]"
                    PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`]` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    CURRENT;
                }
            }

            CURRENT;
            // assert "="
            PARSER_ASSERT(lexeme.type == TOK_EQUAL, "`=` expected but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;

            Expr *initializer = parser_parseExpr(parser, currentScope);
            letDecl->initializer = initializer;
            // assert initializer is not null
            PARSER_ASSERT(initializer != NULL, "Invalid symbol `%s` while parsing variable init value", token_type_to_string(lexeme.type));
            // add the decl to the let uhs
            vec_push(&let->letList, letDecl);

            CURRENT;
            // check if we have a comma
            if (lexeme.type == TOK_COMMA) {
                ACCEPT;
                CURRENT;
            } else {
                parser_reject(parser);
                loop_over_expr = 0;
            }
        }

        // assert "in"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_IN, "`in` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        // parse in uhs
        Expr *inExpr = parser_parseExpr(parser, let->scope);
        let->inExpr = inExpr;
        // assert in uhs is not null
        PARSER_ASSERT(inExpr != NULL, "Invalid symbol `%s` while parsing `in` expression", token_type_to_string(lexeme.type));

        return expr;
    }
    parser_reject(parser);
    return parser_parseMatchExpr(parser, currentScope);
}

// "match" uhs "{" <cases> "}"
Expr* parser_parseMatchExpr(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    if(lexeme.type != TOK_MATCH) {
        parser_reject(parser);
        return parser_parseOpAssign(parser, currentScope);
    }

    ACCEPT;
    Expr* expr = ast_expr_makeExpr(ET_MATCH, lexeme);
    MatchExpr* match = ast_expr_makeMatchExpr(parser_parseExpr(parser, currentScope));
    expr->matchExpr = match;
    // assert "{"
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    uint8_t loop = 1;

    while(loop){

        Expr* condition = parser_parseExpr(parser, currentScope);
        // assert condition is not null
        PARSER_ASSERT(condition != NULL, "Invalid symbol `%s` while parsing match condition", token_type_to_string(lexeme.type));

        CURRENT;
        // assert "=>"
        PARSER_ASSERT(lexeme.type == TOK_CASE_EXPR, "`=>` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        Expr* result = parser_parseExpr(parser, currentScope);
        // assert result is not null
        PARSER_ASSERT(result != NULL, "Invalid symbol `%s` while parsing case expression", token_type_to_string(lexeme.type));
        CaseExpr* case_ = ast_expr_makeCaseExpr(condition, result);

        CURRENT;

        // add case to match
        vec_push(&match->cases, case_);

        // if not comma then exit
        if(lexeme.type != TOK_COMMA){
            parser_reject(parser);
            loop = 0;
        }
        else{
            ACCEPT;
        }
    }
    // assert "}"
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    return expr;
}

Expr* parser_parseOpAssign(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpOr(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(
            lexeme.type == TOK_EQUAL ||
            lexeme.type == TOK_PLUS_EQUAL ||
            lexeme.type == TOK_MINUS_EQUAL ||
            lexeme.type == TOK_STAR_EQUAL ||
            lexeme.type == TOK_DIV_EQUAL
    ) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_ASSIGN, NULL, NULL);

        if(lexeme.type == TOK_EQUAL)
            binaryExpr->binaryExpr->type = BET_ASSIGN;
        else if(lexeme.type == TOK_PLUS_EQUAL)
            binaryExpr->binaryExpr->type = BET_ADD_ASSIGN;
        else if(lexeme.type == TOK_MINUS_EQUAL)
            binaryExpr->binaryExpr->type = BET_SUB_ASSIGN;
        else if(lexeme.type == TOK_STAR_EQUAL)
            binaryExpr->binaryExpr->type = BET_MUL_ASSIGN;
        else if(lexeme.type == TOK_DIV_EQUAL)
            binaryExpr->binaryExpr->type = BET_DIV_ASSIGN;
        else
            // assert 0==1
            PARSER_ASSERT(0==1, "This is a parser error");


        Expr* rhs = parser_parseOpAssign(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }
    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpOr(Parser* parser, ASTScope* currentScope) {
    Expr* lhs = parser_parseOpAnd(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_LOGICAL_OR) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_OR, NULL, NULL);

        Expr* rhs = parser_parseOpOr(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpAnd(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpBinOr(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_LOGICAL_AND) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_AND, NULL, NULL);

        Expr* rhs = parser_parseOpAnd(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpBinOr(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpBinXor(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_BITWISE_OR) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_BIT_OR, NULL, NULL);

        Expr* rhs = parser_parseOpBinOr(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpBinXor(Parser* parser, ASTScope* currentScope){
    Expr* lhs =  parser_parseOpBinAnd(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_BITWISE_XOR) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr =ast_expr_makeBinaryExpr(BET_BIT_XOR, NULL, NULL);

        Expr* rhs = parser_parseOpBinXor(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpBinAnd(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpEq(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_BITWISE_AND) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_BIT_AND, NULL, NULL);

        Expr* rhs = parser_parseOpBinAnd(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpEq(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpCompare(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_EQUAL_EQUAL || lexeme.type == TOK_NOT_EQUAL) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(lexeme.type == TOK_EQUAL_EQUAL?BET_EQ:BET_NEQ, NULL, NULL);

        Expr* rhs = parser_parseOpEq(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

uint8_t lookUpGenericFunctionCall(Parser* parser){
    // will look into the next elements, it skips nested <>, (), [], {}.
    // It will return 1 if it finds a consecutive ">" "(" within the same scope
    vec_char_t stack;
    vec_init(&stack);
    vec_push(&stack, TOK_LESS);
    uint8_t prevWasGreater = 0;
    while(1){
        Lexeme CURRENT;
        if(lexeme.type == TOK_LESS ||
            lexeme.type == TOK_LBRACE ||
            lexeme.type == TOK_LPAREN ||
            lexeme.type == TOK_LBRACKET){

            if(prevWasGreater && lexeme.type == TOK_LPAREN && stack.length == 0){
                return 1;
            }
            vec_push(&stack, lexeme.type);
        }
        else if (lexeme.type == TOK_GREATER || lexeme.type == TOK_RBRACE ||
                 lexeme.type == TOK_RPAREN ||  lexeme.type == TOK_RBRACKET){
            if(lexeme.type == TOK_GREATER){
                prevWasGreater = 1;
            }
            vec_pop(&stack);

        }else if(lexeme.type == TOK_EOF){
            parser_reject(parser);
            return 0;
        }
        else {
            prevWasGreater = 0;
        }
    }
    return 0;
}

Expr* parser_parseOpCompare(Parser* parser, ASTScope* currentScope) {
    Expr* lhs = parser_parseOpShift(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_LESS || lexeme.type == TOK_GREATER ||
    lexeme.type == TOK_LESS_EQUAL || lexeme.type == TOK_GREATER_EQUAL) {
        ACCEPT;
        if(lexeme.type == TOK_LESS && lookUpGenericFunctionCall(parser)) {
            parser_reject(parser);
            CURRENT; // <
            //ACCEPT;

            // make call expression
            Expr* callExpr = ast_expr_makeExpr(ET_CALL, lexeme);
            callExpr->callExpr = ast_expr_makeCallExpr(lhs);
            callExpr->callExpr->hasGenerics = 1;

            // parse generic arguments
            uint8_t can_loop = lexeme.type != TOK_GREATER;
            while(can_loop) {
                DataType * type = parser_parseTypeUnion(parser, NULL, currentScope);
                vec_push(&callExpr->callExpr->generics, type);

                CURRENT;
                if(lexeme.type == TOK_COMMA) {
                    ACCEPT;
                }
                else {
                    PARSER_ASSERT(lexeme.type == TOK_GREATER, "`>` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    can_loop = 0;
                }
            }

            // assert that the next token is a (
            CURRENT;
            PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));

            ACCEPT;
            CURRENT;
            can_loop = lexeme.type != TOK_RPAREN;

            while(can_loop) {
                Expr* index = parser_parseExpr(parser, currentScope);
                vec_push(&callExpr->callExpr->args, index);
                CURRENT;
                if(lexeme.type == TOK_COMMA) {
                    ACCEPT;
                }
                else {
                    PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    can_loop = 0;
                }
            }
            ACCEPT;

            // todo check datatype
            return callExpr;
        }


        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(BET_LT, NULL, NULL);

        if(lexeme.type == TOK_GREATER)
            binaryExpr->binaryExpr->type = BET_GT;
        else if(lexeme.type == TOK_LESS_EQUAL)
            binaryExpr->binaryExpr->type = BET_LTE;
        else if(lexeme.type == TOK_GREATER_EQUAL)
            binaryExpr->binaryExpr->type = BET_GTE;


        Expr* rhs = parser_parseOpCompare(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    else if (lexeme.type == TOK_TYPE_CONVERSION) {
        ACCEPT;
        Expr* castExpr = ast_expr_makeExpr(ET_CAST, lexeme);
        castExpr->castExpr = ast_expr_makeCastExpr(NULL, NULL);
        castExpr->castExpr->expr = lhs;
        castExpr->castExpr->type = parser_parseTypeUnion(parser, NULL, currentScope);
        return castExpr;
    }

    else if (lexeme.type == TOK_IS) {
        ACCEPT;
        Expr* checkExpr = ast_expr_makeExpr(ET_INSTANCE_CHECK, lexeme);
        checkExpr->instanceCheckExpr = ast_expr_makeInstanceCheckExpr(NULL, NULL);
        checkExpr->instanceCheckExpr->expr = lhs;
        checkExpr->instanceCheckExpr->type = parser_parseTypeUnion(parser, NULL, currentScope);
        return  checkExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpShift(Parser* parser, ASTScope* currentScope) {
    Expr* lhs = parser_parseAdd(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_RIGHT_SHIFT || lexeme.type == TOK_LEFT_SHIFT) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(lexeme.type == TOK_RIGHT_SHIFT?BET_RSHIFT:BET_LSHIFT, NULL, NULL);

        Expr* rhs = parser_parseOpShift(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseAdd(Parser* parser, ASTScope* currentScope){
    Expr* lhs = parser_parseOpMult(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_PLUS || lexeme.type == TOK_MINUS) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(lexeme.type == TOK_PLUS?BET_ADD:BET_SUB, NULL, NULL);

        Expr* rhs = parser_parseAdd(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpMult(Parser* parser, ASTScope* currentScope) {
    Expr* lhs = parser_parseOpUnary(parser, currentScope);
    if(lhs == NULL){
        return NULL;
    }
    Lexeme CURRENT;
    if(lexeme.type == TOK_STAR || lexeme.type == TOK_DIV || lexeme.type == TOK_PERCENT) {
        ACCEPT;
        Expr *binaryExpr = ast_expr_makeExpr(ET_BINARY, lexeme);
        binaryExpr->binaryExpr = ast_expr_makeBinaryExpr(lexeme.type == TOK_STAR?BET_MUL:BET_DIV, NULL, NULL);
        if(lexeme.type == TOK_PERCENT)
            binaryExpr->binaryExpr->type = BET_MOD;

        Expr* rhs = parser_parseOpMult(parser, currentScope);
        binaryExpr->binaryExpr->lhs = lhs;
        binaryExpr->binaryExpr->rhs = rhs;

        // Todo: compute data type
        //binaryExpr->dataType

        return binaryExpr;
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpUnary(Parser* parser, ASTScope* currentScope) {
    // check if we have prefix op
    Lexeme CURRENT;
    if (lexeme.type == TOK_STAR || lexeme.type == TOK_MINUS || lexeme.type == TOK_BITWISE_NOT ||
        lexeme.type == TOK_NOT || lexeme.type == TOK_INCREMENT || lexeme.type == TOK_DECREMENT ||
        lexeme.type == TOK_DENULL || lexeme.type == TOK_BITWISE_AND) {
        ACCEPT;
        Expr *unaryExpr = ast_expr_makeExpr(ET_UNARY, lexeme);

        unaryExpr->unaryExpr = ast_expr_makeUnaryExpr(UET_DEREF, NULL);
        if(lexeme.type == TOK_MINUS)
            unaryExpr->unaryExpr->type = UET_NEG;
        else if(lexeme.type == TOK_BITWISE_NOT)
            unaryExpr->unaryExpr->type = UET_BIT_NOT;
        else if(lexeme.type == TOK_NOT)
            unaryExpr->unaryExpr->type = UET_NOT;
        else if(lexeme.type == TOK_INCREMENT)
            unaryExpr->unaryExpr->type = UET_PRE_INC;
        else if(lexeme.type == TOK_DECREMENT)
            unaryExpr->unaryExpr->type = UET_PRE_DEC;
        else if(lexeme.type == TOK_DENULL)
            unaryExpr->unaryExpr->type = UET_DENULL;
        else if(lexeme.type == TOK_BITWISE_AND)
            unaryExpr->unaryExpr->type = UET_ADDRESS_OF;

        Expr *uhs = parser_parseOpUnary(parser, currentScope);
        unaryExpr->unaryExpr->uhs = uhs;

        return unaryExpr;
    }
    else if (lexeme.type == TOK_NEW) {
        Expr* new = ast_expr_makeExpr(ET_NEW, lexeme);
        ACCEPT;
        // parse type
        DataType* dt = parser_parseTypeUnion(parser, NULL, currentScope);
        new->newExpr = ast_expr_makeNewExpr(dt);

        // assert dt not NULL
        PARSER_ASSERT(dt != NULL, "Invalid symbol `%s` while parsing `new` expression", token_type_to_string(lexeme.type));
        CURRENT;
        // check for "("
        if(lexeme.type == TOK_LPAREN) {
            ACCEPT;
            CURRENT;
            uint8_t can_loop = lexeme.type != TOK_RPAREN;
            while(can_loop){
                Expr* arg = parser_parseExpr(parser, currentScope);
                // assert arg not null
                PARSER_ASSERT(arg != NULL, "Invalid symbol `%s` while parsing `new` expression arguments", token_type_to_string(lexeme.type));

                vec_push(&new->newExpr->args, arg);

                CURRENT;
                if(lexeme.type == TOK_COMMA) {
                    ACCEPT;
                }
                else {
                    // check for ")"
                    PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but `%s` was found", token_type_to_string(lexeme.type));
                    can_loop = 0;
                }
            }
            ACCEPT;

            return new;
        }
        else {
            parser_reject(parser);
            return NULL;
        }
    }
    else if (lexeme.type == TOK_SPAWN){
        // prepare expr
        Expr* spawn = ast_expr_makeExpr(ET_SPAWN, lexeme);
        ACCEPT;
        // prepare spawn struct
        spawn->spawnExpr = ast_expr_makeSpawnExpr();
        CURRENT;
        if(lexeme.type == TOK_PROCESS_LINK){
            ACCEPT;
            spawn->spawnExpr->expr = parser_parseExpr(parser, currentScope);
            return spawn;
        }
        else {
            parser_reject(parser);
            spawn->spawnExpr->callback = parser_parseExpr(parser, currentScope);
            CURRENT;
            PARSER_ASSERT(lexeme.type == TOK_PROCESS_LINK, "`->` expected but `%s` was found", token_type_to_string(lexeme.type));
            ACCEPT;
            spawn->spawnExpr->expr = parser_parseExpr(parser, currentScope);
            // TODO: assert callback is valid and expr is of type process

            return spawn;
        }
    }
    else if (lexeme.type == TOK_EMIT){
        Expr* emit = ast_expr_makeExpr(ET_EMIT, lexeme);
        ACCEPT;
        emit->emitExpr = ast_expr_makeEmitExpr();
        CURRENT;
        if(lexeme.type == TOK_PROCESS_LINK){
            ACCEPT;
            emit->emitExpr->msg = parser_parseExpr(parser, currentScope);
            return emit;
        }
        else {
            parser_reject(parser);
            emit->emitExpr->process = parser_parseExpr(parser, currentScope);
            CURRENT;
            PARSER_ASSERT(lexeme.type == TOK_PROCESS_LINK, "`->` expected but `%s` was found", token_type_to_string(lexeme.type));
            ACCEPT;
            emit->emitExpr->msg = parser_parseExpr(parser, currentScope);
            return  emit;
        }
    }

    parser_reject(parser);
    Expr* uhs = parser_parseOpPointer(parser, currentScope);
    Expr *unaryExpr = ast_expr_makeExpr(ET_UNARY, lexeme);
    CURRENT;

    if(lexeme.type == TOK_INCREMENT || lexeme.type == TOK_DECREMENT) {
        ACCEPT;
        unaryExpr->unaryExpr = ast_expr_makeUnaryExpr(lexeme.type == TOK_INCREMENT?UET_POST_INC:UET_POST_DEC, NULL);
        unaryExpr->unaryExpr->uhs = uhs;

        return unaryExpr;
    }

    parser_reject(parser);
    return uhs;
}


Expr* parser_parseMemberAccess(Parser* parser, ASTScope* currentScope, Expr* lhs) {
    Lexeme CURRENT;

    if (lexeme.type == TOK_DOT) {
        ACCEPT;
        Expr* rhs = parser_parseOpValue(parser, currentScope);
        MemberAccessExpr* memberAccessExpr = ast_expr_makeMemberAccessExpr(lhs, rhs);

        Expr* memberExpr = ast_expr_makeExpr(ET_MEMBER_ACCESS, lexeme);
        memberExpr->memberAccessExpr = memberAccessExpr;

        return parser_parseMemberAccess(parser, currentScope, memberExpr);
    }

    if (lexeme.type == TOK_LBRACKET) {
        ACCEPT;
        IndexAccessExpr* idx = ast_expr_makeIndexAccessExpr(lhs);
        uint8_t can_loop = 1;

        while (can_loop) {
            Expr* index = parser_parseExpr(parser, currentScope);
            vec_push(&idx->indexes, index);
            CURRENT;
            if (lexeme.type == TOK_COMMA) {
                ACCEPT;
            }
            else {
                can_loop = 0;
                parser_reject(parser);
            }
        }

        // assert ]
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_RBRACKET, "`]` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        Expr* expr = ast_expr_makeExpr(ET_INDEX_ACCESS, lexeme);
        expr->indexAccessExpr = idx;

        return parser_parseMemberAccess(parser, currentScope, expr);
    }

    if (lexeme.type == TOK_LPAREN) {
        ACCEPT;
        CURRENT;
        CallExpr* call = ast_expr_makeCallExpr(lhs);
        uint8_t can_loop = lexeme.type != TOK_RPAREN;

        while (can_loop) {
            parser_reject(parser);
            Expr* index = parser_parseExpr(parser, currentScope);
            vec_push(&call->args, index);
            CURRENT;
            if (lexeme.type == TOK_COMMA) {
                ACCEPT;
            }
            else {
                PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                can_loop = 0;
            }
        }
        ACCEPT;

        Expr* expr = ast_expr_makeExpr(ET_CALL, lexeme);
        expr->callExpr = call;

        return parser_parseMemberAccess(parser, currentScope, expr);
    }

    parser_reject(parser);
    return lhs;
}

Expr* parser_parseOpPointer(Parser* parser, ASTScope* currentScope) {
    Expr* lhs = parser_parseOpValue(parser, currentScope);
    if (lhs == NULL) {
        return NULL;
    }

    return parser_parseMemberAccess(parser, currentScope, lhs);
}

Expr* parser_parseOpValue(Parser* parser, ASTScope* currentScope) {
    Lexeme CURRENT;
    if(lexeme.type == TOK_IDENTIFIER){
        Expr* expr = ast_expr_makeExpr(ET_ELEMENT, lexeme);
        expr->elementExpr = ast_expr_makeElementExpr(lexeme.string);
        //PARSER_ASSERT(scope_lookupSymbol(currentScope, lexeme.string), "Symbol `%s` is not defined.", lexeme.string);
        /*DataType* type = scope_lookupVariable(currentScope, lexeme.string);
        if(type == NULL)
            type = scope_lookupFunction(currentScope, lexeme.string);
        expr->dataType = type;
        if(type != NULL) {
            // TODO: Remove debug
            printf("SYMBOL %s TYPE %s\n", lexeme.string, ast_json_serializeDataType(type));
        }
        else {
            printf("SYMBOL %s NO TYPE\n", lexeme.string);
        }*/
        ACCEPT;

        return expr;
    }
    if(lexeme.type == TOK_THIS){
        Expr* expr = ast_expr_makeExpr(ET_THIS, lexeme);
        expr->thisExpr = ast_expr_makeThisExpr();
        PARSER_ASSERT(currentScope->withinClass, "`this` can only be used within a class");
        expr->dataType = scope_getClassRef(currentScope);
        PARSER_ASSERT(expr->dataType != NULL, "couldn't get base class of `this`");
        ACCEPT;

        return expr;
    }
    if(lexeme.type == TOK_FN){
        // lambda expression
        parser_reject(parser);
        FnHeader * fnHeader= parser_parseLambdaFnHeader(parser, NULL, currentScope);

        Expr* expr = ast_expr_makeExpr(ET_LAMBDA, lexeme);
        expr->lambdaExpr = ast_expr_makeLambdaExpr(currentScope);
        expr->lambdaExpr->header = fnHeader;
        /**
         * TODO: add args to scope and detect closures
         */

        CURRENT;
        if(lexeme.type == TOK_EQUAL){
            expr->lambdaExpr->bodyType = FBT_EXPR;
            ACCEPT;
            expr->lambdaExpr->expr = parser_parseExpr(parser, expr->lambdaExpr->scope);
            expr->dataType = ti_fnheader_toType(parser, expr->lambdaExpr->scope, expr->lambdaExpr->header, lexeme);
            return expr;
        }
        else{
            // assert {
            PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` or `=` expected but %s was found.", token_type_to_string(lexeme.type));
            expr->lambdaExpr->bodyType = FBT_BLOCK;
            parser_reject(parser);
            expr->lambdaExpr->block = parser_parseStmtBlock(parser, expr->lambdaExpr->scope);
            return expr;
        }

    }
    if(lexeme.type == TOK_LPAREN) {
        ACCEPT;

        Expr* expr = parser_parseExpr(parser, currentScope);
        // assert )
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        return expr;
    }
    // check for array construction [
    if(lexeme.type == TOK_LBRACKET) {
        ACCEPT;
        CURRENT;
        ArrayConstructionExpr* arrayConstructionExpr = ast_expr_makeArrayConstructionExpr();
        uint8_t can_loop = lexeme.type != TOK_RBRACKET;

        while(can_loop) {
            Expr* index = parser_parseExpr(parser, currentScope);
            vec_push(&arrayConstructionExpr->args, index);
            CURRENT;
            if(lexeme.type == TOK_COMMA) {
                ACCEPT;
            }
            else {
                can_loop = 0;
                parser_reject(parser);
                CURRENT;
            }
        }
        // assert ]
        PARSER_ASSERT(lexeme.type == TOK_RBRACKET, "`]` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        Expr* expr = ast_expr_makeExpr(ET_ARRAY_CONSTRUCTION, lexeme);
        expr->arrayConstructionExpr = arrayConstructionExpr;

        // todo check datatype
        return expr;
    }
    // check if we have a "{"
    if(lexeme.type == TOK_LBRACE) {
        ACCEPT;
        // we need to look a head find an ID and ":", then it's named
        // else its unnamed
        CURRENT;
        if(lexeme.type == TOK_IDENTIFIER){
            CURRENT;
            if(lexeme.type == TOK_COLON){
                // build expressions
                NamedStructConstructionExpr* namedStruct = ast_expr_makeNamedStructConstructionExpr();
                Expr* expr = ast_expr_makeExpr(ET_NAMED_STRUCT_CONSTRUCTION, lexeme);
                expr->namedStructConstructionExpr = namedStruct;

                parser_reject(parser);
                // we are back at the identifier
                uint8_t loop = 1;
                while(loop) {
                    CURRENT;
                    // we make sure format is <id>":"<expr> (","<id>":"<expr>)*
                    // we parse the id
                    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
                    char* argName = strdup(lexeme.string);
                    ACCEPT;
                    // we assert ":"
                    CURRENT;
                    PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;

                    Expr* value = parser_parseExpr(parser, currentScope);
                    // we add the arg to the struct
                    vec_push(&namedStruct->argNames, argName);
                    map_set(&namedStruct->args, argName, value);
                    // we check if we have a "," or a "}"
                    CURRENT;
                    if(lexeme.type == TOK_COMMA) {
                        ACCEPT;
                    }
                    else if(lexeme.type == TOK_RBRACE) {
                        ACCEPT;
                        loop = 0;
                    }
                }

                return expr;
            }
            else {
                parser_reject(parser);
            }
        }
        else{
            parser_reject(parser);
        }

        // unnamed struct
        UnnamedStructConstructionExpr* unnamedStruct = ast_expr_makeUnnamedStructConstructionExpr();
        Expr* expr = ast_expr_makeExpr(ET_UNNAMED_STRUCT_CONSTRUCTION, lexeme);
        // prepare loop
        uint8_t can_loop = 1;
        while(can_loop) {
            Expr* index = parser_parseExpr(parser, currentScope);
            vec_push(&unnamedStruct->args, index);
            CURRENT;
            if(lexeme.type == TOK_COMMA) {
                ACCEPT;
            }
            else {
                // assert }
                PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                can_loop = 0;
                parser_reject(parser);
            }
        }
        expr->unnamedStructConstructionExpr = unnamedStruct;
        return expr;
    }
    if(lexeme.type == TOK_UNSAFE){
        // build unsafe expr
        UnsafeExpr* unsafeExpr = ast_expr_makeUnsafeExpr(currentScope);
        Expr* expr = ast_expr_makeExpr(ET_UNSAFE, lexeme);
        expr->unsafeExpr = unsafeExpr;

        ACCEPT;
        // assert "("
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        expr->unsafeExpr->expr = parser_parseExpr(parser, currentScope);
        // assert ")"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        return expr;
    }
    if(lexeme.type == TOK_SYNC){
        // build unsafe expr
        SyncExpr * syncExpr = ast_expr_makeSyncExpr(currentScope);
        Expr* expr = ast_expr_makeExpr(ET_SYNC, lexeme);
        expr->syncExpr = syncExpr;

        ACCEPT;
        // assert "("
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        expr->syncExpr->expr = parser_parseExpr(parser, currentScope);
        // assert ")"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        return expr;
    }
    if(lexeme.type == TOK_IF) {
        // create IfElseExpr
        IfElseExpr* ifElseExpr = ast_expr_makeIfElseExpr();
        ACCEPT;
        ifElseExpr->condition = parser_parseExpr(parser, currentScope);
        // assert {
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        ifElseExpr->ifExpr = parser_parseExpr(parser, currentScope);
        // assert }
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        CURRENT;
        // assert else
        PARSER_ASSERT(lexeme.type == TOK_ELSE, "`else` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        // assert {
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        ifElseExpr->elseExpr = parser_parseExpr(parser, currentScope);
        CURRENT;
        // assert }
        PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`}` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        // build expr
        Expr* expr = ast_expr_makeExpr(ET_IF_ELSE, lexeme);
        expr->ifElseExpr = ifElseExpr;
        return expr;
    }
    parser_reject(parser);
    return parser_parseLiteral(parser, currentScope);
}


/*
    TOK_STRING_VAL,            // string literal  (double quotes)
    TOK_CHAR_VAL,              // single character (single quote)
    TOK_INT,
    TOK_BINARY_INT,         // 0b[01]+
    TOK_OCT_INT,           // 0o[0-7]+
    TOK_HEX_INT,           // 0x[0-9A-F]+
    TOK_FLOAT,             //
    TOK_DOUBLE,
 */
Expr* parser_parseLiteral(Parser* parser, ASTScope* currentScope) {
    Expr* expr = ast_expr_makeExpr(ET_LITERAL,  parser->stack.data[0]);
    expr->dataType = ast_type_makeType(currentScope, parser->stack.data[0], DT_UNRESOLVED);

    expr->literalExpr = ast_expr_makeLiteralExpr(0);
    Lexeme lexeme = parser_peek(parser);

    ACCEPT;
    switch(lexeme.type){
        case TOK_STRING_VAL:
            expr->literalExpr->type = LT_STRING;
            expr->dataType->kind = DT_STRING;
            break;
        case TOK_CHAR_VAL:
            expr->literalExpr->type = LT_CHARACTER;
            expr->dataType->kind = DT_CHAR;
            break;
        case TOK_INT:
            expr->literalExpr->type = LT_INTEGER;
            // TODO maybe check the value to compute an accurate type ?
            expr->dataType->kind = DT_I32;
            break;
        case TOK_BINARY_INT:
            expr->literalExpr->type = LT_BINARY_INT;
            expr->dataType->kind = DT_U32;
            break;
        case TOK_OCT_INT:
            expr->literalExpr->type = LT_OCTAL_INT;
            expr->dataType->kind = DT_U32;
            break;
        case TOK_HEX_INT:
            expr->literalExpr->type = LT_HEX_INT;
            expr->dataType->kind = DT_U32;
            break;
        case TOK_FLOAT:
            expr->literalExpr->type = LT_FLOAT;
            expr->dataType->kind = DT_F32;
            break;
        case TOK_DOUBLE:
            expr->literalExpr->type = LT_DOUBLE;
            expr->dataType->kind = DT_F64;
            break;
        case TOK_TRUE:
            expr->literalExpr->type = LT_BOOLEAN;
            expr->dataType->kind = DT_BOOL;
            expr->literalExpr->value = strdup("true");
            return  expr;
        case TOK_FALSE:
            expr->literalExpr->type = LT_BOOLEAN;
            expr->dataType->kind = DT_BOOL;
            expr->literalExpr->value = strdup("false");
            return expr;
        default:
            // TODO: free memory
            return NULL;
    }
    expr->literalExpr->value = strdup(lexeme.string);
    return expr;
}

/*
 * Terminal parsers
*/

PackageID* parser_parsePackage(Parser* parser, ASTScope* currentScope) {
    PackageID * package = ast_makePackageID();
    Lexeme lexeme = parser_peek(parser);
    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "`identifier/package` expected but %s was found.", token_type_to_string(lexeme.type));
    vec_push(&package->ids, strdup(lexeme.string));

    ACCEPT;

    lexeme = parser_peek(parser);
    uint8_t can_go = lexeme.type == TOK_DOT;
    while(can_go) {
        lexeme = parser_peek(parser);
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "`identifier/package` expected but %s was found.", token_type_to_string(lexeme.type));
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
FnHeader* parser_parseFnHeader(Parser* parser, ASTScope* currentScope) {
    // build header struct
    FnHeader* header = ast_makeFnHeader();
    header->type = ast_type_makeFn();
    // assert we are at "fn"
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_FN, "`fn` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    // assert we got a name
    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    header->name = strdup(lexeme.string);
    // accept name
    ACCEPT;

    // if we have "<" it's a generic
    CURRENT;
    if(lexeme.type == TOK_LESS) {
        header->isGeneric = 1;
        ACCEPT;
        // get current lexeme
        CURRENT;

        // prepare to loop
        uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
        while(can_loop) {
            while(can_loop) {
                // init generic param
                GenericParam * genericParam = ast_make_genericParam();

                // in a type declaration, all given templates in the referee are generic and not concrete.
                PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                              "identifier expected but %s was found.", token_type_to_string(lexeme.type));

                // get generic name
                genericParam->name = strdup(lexeme.string);
                PARSER_ASSERT(scope_fnheader_addGeneric(header, genericParam), "generic param `%s` already exists in function `%s`.", genericParam->name, header->name);
                ACCEPT;
                CURRENT;
                // check if have ":"
                if(lexeme.type == TOK_COLON) {
                    ACCEPT;
                    CURRENT;
                    // get generic type
                    genericParam->constraint = parser_parseTypeUnion(parser, NULL, currentScope);
                    CURRENT;
                }
                else {
                    // if not, set to any
                    genericParam->constraint = NULL;
                }

                if(lexeme.type == TOK_GREATER){
                    can_loop = 0;
                    ACCEPT;
                    CURRENT;
                }

                else {
                    PARSER_ASSERT(lexeme.type == TOK_COMMA,
                                  "`,` expected but %s was found.", token_type_to_string(lexeme.type));
                    ACCEPT;
                    CURRENT;
                }
            }
        }
    }
    // assert we have "("
    PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected after function name but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    // we are going to parse the arguments
    CURRENT;
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    // create fnHeader object
    while(can_loop) {
        // PARSER_ASSERT ID
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected for arg declaration but %s was found.", token_type_to_string(lexeme.type));
        FnArgument * arg = ast_type_makeFnArgument();
        arg->isMutable = 0;
        arg->name = strdup(lexeme.string);
        PARSER_ASSERT(scope_fnheader_addArg(header, arg), "argument name `%s` already exists.", lexeme.string);

        // accept ID
        ACCEPT;
        CURRENT;
        // assert ":"
        PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected after arg name but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        CURRENT;
        // assert type
        DataType* type = parser_parseTypeUnion(parser, NULL, currentScope);
        arg->type = type;

        // check if we reached ")"
        CURRENT;
        if(lexeme.type == TOK_RPAREN) {
            break;

        }
        else {
            PARSER_ASSERT(lexeme.type == TOK_COMMA, "`,` or `)` expected after arg declaration but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;
            CURRENT;
        }
    }

    // do we have a return type?
    ACCEPT;
    CURRENT;
    if(lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        header->type->returnType = parser_parseTypeUnion(parser, NULL, currentScope);
    }
    else{
        parser_reject(parser);
    }

    return header;
}

FnHeader* parser_parseLambdaFnHeader(Parser* parser, DataType* parentReferee, ASTScope* currentScope) {
    // build header struct
    FnHeader* header = ast_makeFnHeader();
    header->type = ast_type_makeFn();
    // assert we are at "fn"
    Lexeme CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_FN, "`fn` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    // if we have "<" it's a generic
    CURRENT;
    if(lexeme.type == TOK_LESS) {
        // GENERICS!
        header->isGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                      "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        uint32_t idx = 0;
        while(can_loop) {
            // init generic param
            GenericParam * genericParam = ast_make_genericParam();

            // in a type declaration, all given templates in the referee are generic and not concrete.
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                          "identifier expected but %s was found.", token_type_to_string(lexeme.type));

            // get generic name
            genericParam->name = strdup(lexeme.string);
            PARSER_ASSERT(scope_fnheader_addGeneric(header, genericParam), "generic param `%s` already exists in anonymous function.", genericParam->name);
            ACCEPT;
            CURRENT;
            // check if have ":"
            if(lexeme.type == TOK_COLON) {
                ACCEPT;
                CURRENT;
                // get generic type
                genericParam->constraint = parser_parseTypeUnion(parser, NULL, currentScope);
                CURRENT;
            }
            else {
                // if not, set to any
                genericParam->constraint = NULL;
            }

            if(lexeme.type == TOK_GREATER){
                can_loop = 0;
                ACCEPT;
                CURRENT;
            }

            else {
                PARSER_ASSERT(lexeme.type == TOK_COMMA,
                              "`,` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                CURRENT;
            }
        }
    }
    // assert we have "("
    PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected after function name but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    // we are going to parse the arguments
    CURRENT;
    uint8_t can_loop = lexeme.type == TOK_IDENTIFIER;
    // create fnHeader object
    while(can_loop) {
        // PARSER_ASSERT ID
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected for arg declaration but %s was found.", token_type_to_string(lexeme.type));
        // accept ID
        char* name = strdup(lexeme.string);

        // make FnArg
        FnArgument * arg = ast_type_makeFnArgument();
        arg->name = name;
        PARSER_ASSERT(scope_fnheader_addArg(header, arg), "argument name `%s` already exists.", name);

        ACCEPT;
        CURRENT;
        // assert ":"
        PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected after arg name but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        CURRENT;
        // assert type
        DataType* type = parser_parseTypeUnion(parser, NULL, currentScope);

        arg->type = type;
        arg->isMutable = 0;

        // check if we reached ")"
        CURRENT;
        if(lexeme.type == TOK_RPAREN) {
            can_loop = 0;
            ACCEPT;
        }
        else {
            PARSER_ASSERT(lexeme.type == TOK_COMMA, "`,` or `)` expected after arg declaration but %s was found.", token_type_to_string(lexeme.type));
            ACCEPT;
            CURRENT;
        }
    }

    // do we have a return type?
    CURRENT;
    if(lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        header->type->returnType = parser_parseTypeUnion(parser, NULL, currentScope);
    }
    else{
        parser_reject(parser);
    }

    return header;
}

Statement* parser_parseStmt(Parser* parser, ASTScope* currentScope) {
    // we check all the options. if no option is chosen, we parse expressions as a statement
    // if we have a return statement
    Lexeme CURRENT;
    // if we have a let statement
    if(lexeme.type == TOK_LET) {
        parser_reject(parser);
        Statement *stmt = parser_parseStmtLet(parser, currentScope);
        // register stmt
        // iterate through stmt->varDecl->letList
        LetExprDecl* varDecl;
        uint32_t i = 0;
        vec_foreach(&stmt->varDecl->letList, varDecl, i){
            // register varDecl->name
            // iterate through varDecl
            char* name;
            uint32_t j = 0;
            vec_foreach(&varDecl->variableNames, name, j){
                FnArgument ** arg = map_get(&varDecl->variables, name);
                PARSER_ASSERT(scope_registerVariable(currentScope, *arg), "variable `%s` already exists in scope.", name);
            }
        }

        return stmt;
    }
    // if we have a function declaration
    else if(lexeme.type == TOK_FN) {
        parser_reject(parser);
        return parser_parseStmtFn(parser, currentScope);
    }
    // if we have an if statement
    else if(lexeme.type == TOK_IF) {
        parser_reject(parser);
        return parser_parseStmtIf(parser, currentScope);
    }
    // if we have a while statement
    else if(lexeme.type == TOK_WHILE) {
        parser_reject(parser);
        return parser_parseStmtWhile(parser, currentScope);
    }
    // check if we have 'do'
    else if(lexeme.type == TOK_DO) {
        parser_reject(parser);
        return parser_parseStmtDoWhile(parser, currentScope);
    }
    // if we have a for statement
    else if(lexeme.type == TOK_FOR) {
        parser_reject(parser);
        return parser_parseStmtFor(parser, currentScope);
    }
    // if we have foreach
    else if(lexeme.type == TOK_FOREACH) {
        parser_reject(parser);
        return parser_parseStmtForEach(parser, currentScope);
    }
    // if we have a break statement
    else if(lexeme.type == TOK_BREAK) {
        parser_reject(parser);
        return parser_parseStmtBreak(parser, currentScope);
    }
    // if we have a continue statement
    else if(lexeme.type == TOK_CONTINUE) {
        parser_reject(parser);
        return parser_parseStmtContinue(parser, currentScope);
    }
    // if we have a return statement
    else if(lexeme.type == TOK_RETURN) {
        parser_reject(parser);
        return parser_parseStmtReturn(parser, currentScope);
    }
    // if we have a block statement
    else if(lexeme.type == TOK_LBRACE) {
        parser_reject(parser);
        return parser_parseStmtBlock(parser, currentScope);
    }
    // if we have a match
    else if(lexeme.type == TOK_MATCH) {
        parser_reject(parser);
        return parser_parseStmtMatch(parser, currentScope);
    }
    // if we have sync
    else if(lexeme.type == TOK_SYNC) {
        parser_reject(parser);
        return parser_parseStmtSync(parser, currentScope);
    }
    else if(lexeme.type == TOK_UNSAFE) {
        parser_reject(parser);
        return parser_parseStmtUnsafe(parser, currentScope);
    }
    // otherwise, we expect expr

    parser_reject(parser);
    return parser_parseStmtExpr(parser, currentScope);
}

Statement* parser_parseStmtLet(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    Statement* stmt = ast_stmt_makeStatement(ST_VAR_DECL, lexeme);
    stmt->varDecl = ast_stmt_makeVarDeclStatement(currentScope);
    // assert let
    PARSER_ASSERT(lexeme.type == TOK_LET, "`let` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    CURRENT;
    uint8_t loop_over_expr = 1;
    while (loop_over_expr) {
        LetExprDecl *letDecl = ast_expr_makeLetExprDecl();
        char *expect = NULL;

        // check if we have id or { or [
        if (lexeme.type == TOK_LBRACE) {
            expect = "]";
            letDecl->initializerType = LIT_STRUCT_DECONSTRUCTION;
            ACCEPT;
            CURRENT;
        } else if (lexeme.type == TOK_LBRACKET) {
            expect = "]";
            letDecl->initializerType = LIT_ARRAY_DECONSTRUCTION;
            ACCEPT;
            CURRENT;
        } else {
            expect = NULL;
            letDecl->initializerType = LIT_NONE;
        }

        uint8_t loop = 1;
        while (loop) {
            FnArgument *var = ast_type_makeFnArgument();
            // check if we have a mut
            if (lexeme.type == TOK_MUT) {
                var->isMutable = 1;
                ACCEPT;
                CURRENT;
            }
            // assert ID
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                   "identifier expected but %s was found.", token_type_to_string(lexeme.type));
            var->name = strdup(lexeme.string);
            ACCEPT;
            CURRENT;
            if(lexeme.type == TOK_COLON){
                // assert ":"
                PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                // parse type
                var->type = parser_parseTypeUnion(parser, NULL, currentScope);
                // assert type is not null
                PARSER_ASSERT(var->type != NULL,"Invalid symbol %s while parsing variable data type", var->name);
                CURRENT;
            }
            // add to args
            map_set(&letDecl->variables, var->name, var);
            vec_push(&letDecl->variableNames, var->name);

            // check if we have a comma

            if (lexeme.type == TOK_COMMA) {
                ACCEPT;
                CURRENT;
            } else {
                parser_reject(parser);
                loop = 0;
            }
        }
        // if we expected something earlier, we must find it closed now
        if (expect != NULL) {
            CURRENT;
            // if we expected "]", we must find "]"
            if (expect[0] == '[') {
                // assert "]"
                PARSER_ASSERT(lexeme.type == TOK_RBRACKET,
                       "`]` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                CURRENT;
            } else if (expect[0] == '{') {
                // assert "]"
                PARSER_ASSERT(lexeme.type == TOK_RBRACE, "`]` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                CURRENT;
            }
        }

        CURRENT;
        // assert "="
        PARSER_ASSERT(lexeme.type == TOK_EQUAL, "`=` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;

        Expr *initializer = parser_parseExpr(parser, currentScope);
        letDecl->initializer = initializer;
        // assert initializer is not null
        PARSER_ASSERT(initializer != NULL, "Invalid symbol %s while parsing variable initializer", token_type_to_string(lexeme.type));
        // add the decl to the let uhs
        vec_push(&stmt->varDecl->letList, letDecl);

        CURRENT;
        // check if we have a comma
        if (lexeme.type == TOK_COMMA) {
            ACCEPT;
            CURRENT;
        } else {
            parser_reject(parser);
            loop_over_expr = 0;
        }
    }
    return stmt;
}

Statement* parser_parseStmtFn(Parser* parser, ASTScope* currentScope) {
    Lexeme CURRENT;

    Statement* stmt = ast_stmt_makeStatement(ST_FN_DECL, lexeme);
    stmt->fnDecl = ast_stmt_makeFnDeclStatement(currentScope);
    // create the header

    // assert fn
    PARSER_ASSERT(lexeme.type == TOK_FN, "`fn` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    CURRENT;
    // assert ID
    PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
    stmt->fnDecl->header->name = strdup(lexeme.string);
    ACCEPT;
    CURRENT;


    if(lexeme.type == TOK_LESS) {
        // GENERICS!
        stmt->fnDecl->header->isGeneric = 1;
        ACCEPT;
        CURRENT;
        uint8_t can_loop = 1;
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
               "identifier expected but %s was found.", token_type_to_string(lexeme.type));

        while(can_loop) {
            // init generic param
            GenericParam * genericParam = ast_make_genericParam();

            // in a type declaration, all given templates in the referee are generic and not concrete.
            PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER,
                   "identifier expected but %s was found.", token_type_to_string(lexeme.type));

            // get generic name
            genericParam->name = strdup(lexeme.string);
            PARSER_ASSERT(scope_fnheader_addGeneric(stmt->fnDecl->header, genericParam),
                   "Generic parameter %s already exists in function %s", genericParam->name, stmt->fnDecl->header->name);
            ACCEPT;
            CURRENT;
            // check if have ":"
            if(lexeme.type == TOK_COLON) {
                ACCEPT;
                CURRENT;
                // get generic type
                genericParam->constraint = parser_parseTypeUnion(parser, NULL, currentScope);
                CURRENT;
            }
            else {
                // if not, set to any
                genericParam->constraint = NULL;
            }

            // add generic param
            // TODO: add api

            if(lexeme.type == TOK_GREATER){
                can_loop = 0;
                ACCEPT;
                CURRENT;
            }

            else {
                PARSER_ASSERT(lexeme.type == TOK_COMMA,
                       "`,` expected but %s was found.", token_type_to_string(lexeme.type));
                ACCEPT;
                CURRENT;
            }
        }
    }

    // assert "("
    PARSER_ASSERT(lexeme.type == TOK_LPAREN, "`(` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;

    CURRENT;
    uint8_t loop = lexeme.type != TOK_RPAREN;
    while (loop) {
        FnArgument *arg = ast_type_makeFnArgument();
        // check if we have a mut
        if (lexeme.type == TOK_MUT) {
            arg->isMutable = 1;
            ACCEPT;
            CURRENT;
        }
        // assert ID
        PARSER_ASSERT(lexeme.type == TOK_IDENTIFIER, "identifier expected but %s was found.", token_type_to_string(lexeme.type));
        arg->name = strdup(lexeme.string);
        ACCEPT;
        CURRENT;
        // assert ":"
        PARSER_ASSERT(lexeme.type == TOK_COLON, "`:` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
        // parse type
        arg->type = parser_parseTypeUnion(parser, NULL, currentScope);
        // assert type is not null
        PARSER_ASSERT(arg->type != NULL,
               "`type` near %s, generated a NULL type. This is a parser issue.");
        // add to args
        map_set(&stmt->fnDecl->header->type->args, arg->name, arg);
        vec_push(&stmt->fnDecl->header->type->argNames, arg->name);

        // check if we have a comma
        lexeme = parser_peek(parser);
        if (lexeme.type == TOK_COMMA) {
            ACCEPT;
            CURRENT;
        } else {
            PARSER_ASSERT(lexeme.type == TOK_RPAREN, "`)` expected but %s was found.", token_type_to_string(lexeme.type));
            loop = 0;
        }
    }
    ACCEPT;
    CURRENT;

    if (lexeme.type == TOK_FN_RETURN_TYPE) {
        ACCEPT;
        // parse the return type
        // TODO: add generics to scope if they exist?
        stmt->fnDecl->header->type->returnType = parser_parseTypeUnion(parser, NULL, currentScope);
        // assert type is not null
        PARSER_ASSERT(stmt->fnDecl->header->type->returnType != NULL,
               "`type` near %s, generated a NULL type. This is a parser issue.");
        ACCEPT;
        CURRENT;

    }

    // check if we have `=` or `{`
    //lexeme = parser_peek(parser);
    if (lexeme.type == TOK_EQUAL) {
        stmt->fnDecl->bodyType = FBT_EXPR;
        ACCEPT;
        //CURRENT;
        // parse the body
        // TODO: update current
        stmt->fnDecl->expr = parser_parseExpr(parser, stmt->fnDecl->scope);
        // assert body is not null
        PARSER_ASSERT(stmt->fnDecl->expr != NULL, "Invalid symbol %s while parsing function expression.");
        //ACCEPT;
        //CURRENT;
    } else if (lexeme.type == TOK_LBRACE) {
        stmt->fnDecl->bodyType = FBT_BLOCK;
        parser_reject(parser);
        stmt->fnDecl->block = parser_parseStmtBlock(parser, stmt->fnDecl->scope);
    } else {
        parser_reject(parser);
        // assert false
        PARSER_ASSERT(0, "`=` or `{` expected but %s was found.", token_type_to_string(lexeme.type));
    }
    PARSER_ASSERT(scope_registerFunction(currentScope, stmt->fnDecl),
           "Function %s already exists in scope", stmt->fnDecl->header->name);

    stmt->fnDecl->dataType = ti_fndecl_toType(parser, currentScope, stmt->fnDecl, lexeme);
    return stmt;
}

Statement* parser_parseStmtBlock(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // create statement instance
    Statement* stmt = ast_stmt_makeStatement(ST_BLOCK, lexeme);
    stmt->blockStmt = ast_stmt_makeBlockStatement(currentScope);
    // assert {
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    uint8_t loop = lexeme.type != TOK_RBRACE;
    if(!loop) {
        ACCEPT;
        return stmt;
    }
    parser_reject(parser);
    while(loop) {
        Statement * s = parser_parseStmt(parser, stmt->blockStmt->scope);
        PARSER_ASSERT(s != NULL, "Invalid symbol %s while parsing block statement.", token_type_to_string(lexeme.type));
        vec_push(&stmt->blockStmt->stmts, s);
        // TODO: free s

        CURRENT;
        if(lexeme.type == TOK_RBRACE || lexeme.type == TOK_EOF){
            ACCEPT;
            loop = 0;
        }
        else {
            loop++;
            parser_reject(parser);
        }
    }

    return stmt;
}

Statement* parser_parseStmtIf(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // create statement instance
    Statement* stmt = ast_stmt_makeStatement(ST_IF_CHAIN, lexeme);
    stmt->ifChain = ast_stmt_makeIfChainStatement();
    // assert if
    PARSER_ASSERT(lexeme.type == TOK_IF, "`if` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    CURRENT;
    // prepare to loop
    uint8_t loop = 1;
    /*
     * if true {} else if true {} else {}
     * we parse it as 2 conditions and 2 blocks, final else is separate
     */
    while (loop) {
        Expr* condition = parser_parseExpr(parser, currentScope);
        // assert condition is not null
        PARSER_ASSERT(condition != NULL, "Invalid symbol %s while parsing if condition", token_type_to_string(lexeme.type));
        // next token


        Statement* block = parser_parseStmtBlock(parser, currentScope);

        vec_push(&stmt->ifChain->conditions, condition);
        vec_push(&stmt->ifChain->blocks, block);

        CURRENT;

        if(lexeme.type == TOK_ELSE){
            ACCEPT;
            CURRENT;
            if(lexeme.type == TOK_IF){
                ACCEPT;
                CURRENT;
            }
            else{
                parser_reject(parser);
                stmt->ifChain->elseBlock = parser_parseStmtBlock(parser, currentScope);
                loop = 0;
            }
        }
        else {
            parser_reject(parser);
            stmt->ifChain->elseBlock = NULL;
            loop = 0;
        }
    }

    return stmt;
}
Statement* parser_parseStmtMatch(Parser* parser, ASTScope* currentScope){
    // get current token
    Lexeme CURRENT;
    // assert match
    PARSER_ASSERT(lexeme.type == TOK_MATCH, "`match` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // create statement instance
    Statement* stmt = ast_stmt_makeStatement(ST_MATCH, lexeme);
    stmt->match = ast_stmt_makeMatchStatement();
    // parse the main expression to be matched
    stmt->match->expr = parser_parseExpr(parser, currentScope);
    // assert expr is not null
    PARSER_ASSERT(stmt->match->expr != NULL, "Invalid symbol %s while parsing match expression.", token_type_to_string(lexeme.type));
    // assert {
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // prepare to loop
    uint8_t loop = 1;
    while(loop) {
        CURRENT;
        // do we have a wildcard
        if (lexeme.type == TOK_WILDCARD){
            ACCEPT;
            stmt->match->elseBlock = parser_parseStmtBlock(parser, currentScope);
            loop = 0;
        }
        else {
            parser_reject(parser);
            // create CaseStatement
            CaseStatement *caseStmt = ast_stmt_makeCaseStatement();
            // parse the condition
            caseStmt->condition = parser_parseExpr(parser, currentScope);
            PARSER_ASSERT(caseStmt->condition != NULL,"Invalid symbol %s while parsing match case condition.", token_type_to_string(lexeme.type));
            // next token
            CURRENT;
            // assert {
            PARSER_ASSERT(lexeme.type == TOK_LBRACE, "`{` expected but %s was found.", token_type_to_string(lexeme.type));
            parser_reject(parser);
            // parse the block
            caseStmt->block = parser_parseStmtBlock(parser, currentScope);
            // assert block is not null
            PARSER_ASSERT(caseStmt->block != NULL,"Invalid symbol %s while parsing match case block.", token_type_to_string(lexeme.type));
            vec_push(&stmt->match->cases, caseStmt);
        }

        CURRENT;
        if(lexeme.type == TOK_RBRACE){
            ACCEPT;
            loop = 0;
        }
        else
            parser_reject(parser);
    }

    return stmt;
}

Statement* parser_parseStmtWhile(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_WHILE, lexeme);
    stmt->whileLoop = ast_stmt_makeWhileStatement(currentScope);
    // assert while
    PARSER_ASSERT(lexeme.type == TOK_WHILE, "`while` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse condition
    stmt->whileLoop->condition = parser_parseExpr(parser, currentScope);
    // assert condition is not null
    PARSER_ASSERT(stmt->whileLoop->condition != NULL, "Invalid symbol %s while parsing while condition.", token_type_to_string(lexeme.type));
    // parse block
    stmt->whileLoop->block = parser_parseStmtBlock(parser, stmt->whileLoop->scope);
    // assert block is not null
    PARSER_ASSERT(stmt->whileLoop->block != NULL, "Invalid symbol %s while parsing while block.", token_type_to_string(lexeme.type));

    return stmt;
}

Statement* parser_parseStmtDoWhile(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_DO_WHILE, lexeme);
    stmt->doWhileLoop = ast_stmt_makeDoWhileStatement(currentScope);
    // assert do
    PARSER_ASSERT(lexeme.type == TOK_DO, "`do` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse block
    stmt->doWhileLoop->block = parser_parseStmtBlock(parser, stmt->doWhileLoop->scope);
    // assert block is not null
    PARSER_ASSERT(stmt->doWhileLoop->block != NULL, "Invalid symbol %s while parsing do-while block.", token_type_to_string(lexeme.type));
    // assert while
    CURRENT;
    PARSER_ASSERT(lexeme.type == TOK_WHILE, "`while` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse condition
    stmt->doWhileLoop->condition = parser_parseExpr(parser, currentScope);
    // assert condition is not null
    PARSER_ASSERT(stmt->doWhileLoop->condition != NULL, "Invalid symbol %s while parsing do-while condition.", token_type_to_string(lexeme.type));
    return stmt;
}

Statement* parser_parseStmtFor(Parser* parser, ASTScope* currentScope) {
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_FOR, lexeme);
    stmt->forLoop = ast_stmt_makeForStatement(currentScope);
    // assert for
    PARSER_ASSERT(lexeme.type == TOK_FOR, "`for` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // make sure we have initializer, i.e token is not ";"
    CURRENT;
    if(lexeme.type != TOK_SEMICOLON){
        parser_reject(parser);
        // parse initializer
        stmt->forLoop->initializer = parser_parseStmtLet(parser, currentScope);

        // assert initializer is not null
        PARSER_ASSERT(stmt->forLoop->initializer != NULL, "Invalid symbol %s while parsing for-loop initializer.", token_type_to_string(lexeme.type));
        // assert token is ";"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_SEMICOLON, "`;` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
    }
    else{
        ACCEPT;
    }
    CURRENT;
    // now we parse the condition
    if(lexeme.type != TOK_SEMICOLON){
        parser_reject(parser);
        // parse condition
        stmt->forLoop->condition = parser_parseExpr(parser, stmt->forLoop->scope);
        // assert condition is not null
        PARSER_ASSERT(stmt->forLoop->condition != NULL,
               "Invalid symbol %s while parsing for-loop condition.", token_type_to_string(lexeme.type));
        // assert token is ";"
        CURRENT;
        PARSER_ASSERT(lexeme.type == TOK_SEMICOLON, "`;` expected but %s was found.", token_type_to_string(lexeme.type));
        ACCEPT;
    }
    else
        ACCEPT;

    // check if we have {
    CURRENT;
    if(lexeme.type != TOK_LBRACE) {

        parser_reject(parser);
        // now we parse increments as <expr>(","<expr>)*
        // prepare to loop
        uint8_t loop = 1;
        while(loop) {
            // parse one increment
            Expr* increment = parser_parseExpr(parser, stmt->forLoop->scope);
            // assert increment is not null
            PARSER_ASSERT(increment != NULL, "Invalid symbol %s while parsing for-loop increment.", token_type_to_string(lexeme.type));
            // add increment to list
            vec_push(&stmt->forLoop->increments, increment);
            // check if we have a comma
            CURRENT;
            if(lexeme.type == TOK_COMMA)
                ACCEPT;
            else
                loop = 0;
        }
    }
    parser_reject(parser);

    // current
    stmt->forLoop->block = parser_parseStmtBlock(parser, stmt->forLoop->scope);
    // assert block is not null
    PARSER_ASSERT(stmt->forLoop->block != NULL, "Invalid symbol %s while parsing for-loop block.", token_type_to_string(lexeme.type));
    return stmt;
}

Statement* parser_parseStmtForEach(Parser* parser, ASTScope* currentScope){
    // throw not implemented error
    Lexeme CURRENT;
    PARSER_ASSERT(0, "`foreach` is not implemented yet.");
}

Statement* parser_parseStmtContinue(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_CONTINUE, lexeme);
    stmt->continueStmt = ast_stmt_makeContinueStatement();
    // assert continue
    PARSER_ASSERT(lexeme.type == TOK_CONTINUE, "`continue` expected but %s was found.", token_type_to_string(lexeme.type));
    PARSER_ASSERT(currentScope->withinLoop, "`continue` statement must be within a loop.");
    ACCEPT;
    return stmt;
}

Statement* parser_parseStmtReturn(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_RETURN, lexeme);
    stmt->returnStmt = ast_stmt_makeReturnStatement();
    // assert return
    PARSER_ASSERT(currentScope->withinFn, "`return` statement must be within a function.");
    PARSER_ASSERT(lexeme.type == TOK_RETURN, "`return` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse expression
    FnHeader * header = scope_getFnRef(currentScope);
    if(header->type->returnType != NULL) {
        stmt->returnStmt->expr = parser_parseExpr(parser, currentScope);
        PARSER_ASSERT(stmt->returnStmt->expr != NULL, "Function `%s` must return an expression", header->name);
    }

    // return can be NULL.

    return stmt;
}

Statement* parser_parseStmtBreak(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_BREAK, lexeme);
    stmt->breakStmt = ast_stmt_makeBreakStatement();
    // assert break
    PARSER_ASSERT(lexeme.type == TOK_BREAK, "`break` expected but %s was found.", token_type_to_string(lexeme.type));
    PARSER_ASSERT(currentScope->withinLoop, "`break` statement must be within a loop.");
    ACCEPT;
    return stmt;
}

Statement* parser_parseStmtUnsafe(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_UNSAFE, lexeme);
    stmt->unsafeStmt = ast_stmt_makeUnsafeStatement(currentScope);
    // assert unsafe
    PARSER_ASSERT(lexeme.type == TOK_UNSAFE, "`unsafe` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse block
    stmt->unsafeStmt->block = parser_parseStmtBlock(parser, stmt->unsafeStmt->scope);
    // assert block is not null
    PARSER_ASSERT(stmt->unsafeStmt->block != NULL, "Invalid symbol %s while parsing unsafe block.", token_type_to_string(lexeme.type));
    return stmt;
}

Statement* parser_parseStmtSync(Parser* parser, ASTScope* currentScope){
    Lexeme CURRENT;
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_SYNC, lexeme);
    stmt->syncStmt = ast_stmt_makeSyncStatement(currentScope);
    // assert sync
    PARSER_ASSERT(lexeme.type == TOK_SYNC, "`unsafe` expected but %s was found.", token_type_to_string(lexeme.type));
    ACCEPT;
    // parse block
    stmt->unsafeStmt->block = parser_parseStmtBlock(parser, currentScope);
    // assert block is not null
    PARSER_ASSERT(stmt->unsafeStmt->block != NULL, "Invalid symbol %s while parsing unsafe block.", token_type_to_string(lexeme.type));
    return stmt;
}

Statement* parser_parseStmtExpr(Parser* parser, ASTScope* currentScope){
    // build statement
    Statement* stmt = ast_stmt_makeStatement(ST_EXPR, parser->stack.data[0]);
    stmt->expr = ast_stmt_makeExprStatement(currentScope);
    // parse expression
    stmt->expr->expr = parser_parseExpr(parser, currentScope);
    // expr can be null

    if(stmt->expr->expr == NULL){
        // TODO free memory
        free(stmt->expr);
        free(stmt);
        return NULL;
    }

    return stmt;
}

#undef CURRENT
#undef ACCEPT