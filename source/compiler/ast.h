//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_AST_H
#define TYPE_C_AST_H

#include "../utils/vec.h"
#include "tokens.h"

typedef struct PackageID {
    vec_str_t ids;
}PackageID;

typedef struct ImportStmt {
    // combines both source and test
    PackageID* path;
    uint8_t hasAlias;
    char* alias;
}ImportStmt;


typedef vec_t(ImportStmt*) import_stmt_vec;


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
    import_stmt_vec importStatements;
}ASTProgramNode;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        ASTProgramNode* programNode;
    };
}ASTNode;


ASTNode * ast_makeProgramNode();
PackageID* ast_makePackageID();
ImportStmt* ast_makeImportStmt(PackageID* source, PackageID* target, uint8_t hasAlias, char* alias);
void ast_debug_programImport(ASTProgramNode* node);

#endif //TYPE_C_AST_H
