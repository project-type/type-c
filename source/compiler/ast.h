//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_AST_H
#define TYPE_C_AST_H

#include "tokens.h"

typedef enum ASTNodeType{
    AST_PROGRAM,
    AST_IMPORT,
    AST_FN,
    AST_VAR_DECL,
    AST_ID,
    AST_BLOCK,
}ASTNodeType;


typedef struct ASTPostFixExpressionNode {
    struct ASTNode* uhs;
    TokenType op;
}ASTPostFixExpressionNode;


typedef struct ASTBinaryExpressionNode {
    struct ASTNode* lhs;
    struct ASTNode* rhs;
    TokenType op;
}BinaryExpressionNode;

typedef struct ASTScope {
    void* variables;
    void* functions;
    void* dataTypes;
    void* statements;
    void* externDeclarations;
    void* expressions;
} ASTScope;

typedef struct ASTProgram {
    const char* name;
    ASTScope* scope;
};

#endif //TYPE_C_AST_H
