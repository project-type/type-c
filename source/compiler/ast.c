//
// Created by praisethemoon on 28.04.23.
//
#include <stdio.h>
#include "ast.h"
#include "../utils/vec.h"

ASTNode * ast_makeProgramNode() {
    ASTProgramNode* program = malloc(sizeof(ASTProgramNode));
    program->scope = NULL;
    vec_init(&program->importStatements);

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->programNode = program;

    return node;
}

PackageID* ast_makePackageID() {
    PackageID* package = malloc(sizeof(PackageID));
    vec_init(&package->ids);

    return package;
}

ImportStmt* ast_makeImportStmt(PackageID* source, PackageID* target, uint8_t hasAlias, char* alias) {
    uint32_t i; char** str;
    PackageID * full_path = ast_makePackageID();
    vec_foreach_ptr(&source->ids, str, i) {
        vec_push(&full_path->ids, strdup(*str));
    }
    vec_foreach_ptr(&target->ids, str, i) {
        vec_push(&full_path->ids, strdup(*str));
    }

    ImportStmt * importStmt = malloc(sizeof(ImportStmt));
    importStmt->hasAlias = hasAlias;
    importStmt->alias = alias != NULL? strdup(alias):NULL;
    importStmt->path = full_path;

    return importStmt;
}

void ast_debug_programImport(ASTProgramNode* node) {
    uint32_t i = 0;
    ImportStmt ** importStmt;
    printf("Total of %d imports found\n", node->importStatements.length);

    vec_foreach_ptr(&node->importStatements, importStmt, i) {
        uint32_t j;
        char** str;
        ImportStmt* stmt = *importStmt;
        printf("[%d](\n", i);
        vec_foreach_ptr(&stmt->path->ids, str, j) {
            printf("\t%s,\n", *str);
        }
        if(stmt->hasAlias){
            printf("\t as %s\n", stmt->alias);
        }
        printf(")\n");
    }
}