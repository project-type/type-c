//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_AST_H
#define TYPE_C_AST_H

#include "../utils/vec.h"
#include "tokens.h"

#define MAKE_NODE(name, nodeDataType, nodeEnum)\
    name = (nodeType*)malloc(sizeof(Node));\
    name->type - nodeEnum;\
    name->

typedef struct PackageID {
    vec_str_t ids;
}PackageID;

typedef enum ASTNodeType{
    AST_PROGRAM,
    AST_IMPORT,
}ASTNodeType;

typedef struct ASTScope {
    void* variables;
    void* functions;
    void* dataTypes;
    void* statements;
    void* externDeclarations;
    void* expressions;
} ASTScope;

typedef struct ASTProgramNode {
    ASTScope* scope;
}ASTProgramNode;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        ASTProgramNode* programNode;
    };
}ASTNode;

typedef struct ImportStmt {
    PackageID * source;
    PackageID * dest;
    uint8_t hasAlias;
    char* alias;
}ImportStmt;


PackageID* ast_makePackageID();

#endif //TYPE_C_AST_H
