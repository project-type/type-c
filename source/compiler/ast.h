//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_AST_H
#define TYPE_C_AST_H

#include "../utils/vec.h"
#include "tokens.h"

typedef enum DataTypes {
    DT_U8,
    DT_I8,
    DT_U16,
    DT_I16,
    DT_U32,
    DT_I32,
    DT_U64,
    DT_I64,
    DT_F32,
    DT_F64,
    DT_STRING,
    DT_CHAR,
    DT_OBJECT,
    DT_CLASS,
    DT_INTERFACE,
    DT_ARRAY,
}DataTypes;

typedef struct TypeKind {
    DataTypes dataType;
    size_t size;

}TypeKind;

typedef struct PackageID {
    vec_str_t ids;
}PackageID;

typedef struct ImportStmt {
    // combines both source and test
    PackageID* path;
    uint8_t hasAlias;
    char* alias;
}ImportStmt;

typedef struct FnArgument {
    char* name;
    uint8_t is_mut;
    uint8_t is_generic;

}FnArgument;

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
