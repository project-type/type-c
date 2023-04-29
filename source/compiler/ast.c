//
// Created by praisethemoon on 28.04.23.
//
#include <stdio.h>
#include "ast.h"
#include "../utils/vec.h"
#include "../utils/map.h"

#define ALLOC(v, t) t* v = malloc(sizeof(t))


ASTNode * ast_makeProgramNode() {
    ASTProgramNode* program = malloc(sizeof(ASTProgramNode));

    vec_init(&program->importStatements);

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->programNode = program;
    map_init(&node->scope.dataTypes);

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

void ast_debug_Type(DataType* type){
    printf("type %s\n", type->name);
    switch(type->kind) {
        case DT_ENUM:
            printf("\t is ENUM");
            break;
        case DT_ARRAY:
            printf("\t is array of size %llu\n", type->arrayType->len);
            ast_debug_Type(type->arrayType->arrayOf);
            break;
        case DT_I8:
            printf("i8\n");
        case DT_I16:
            printf("i16\n");
            break;
        case DT_I32:
            printf("i32\n");
            break;
        case DT_I64:
            printf("i64\n");
            break;
        case DT_U8:
            printf("u8\n");
            break;
        case DT_U16:
            printf("u16\n");
            break;
        case DT_U32:
            printf("u32\n");
            break;
        case DT_U64:
            printf("u64\n");
            break;
        case DT_F32:
            printf("f32\n");
            break;
        case DT_F64:
            printf("f32\n");
            break;
        case DT_BOOL:
            printf("bool\n");
            break;
        case DT_STRING:
            printf("string\n");
            break;
        case DT_CHAR:
            printf("char\n");
            break;
        case DT_CLASS:
            printf("class\n");
            break;
        case DT_INTERFACE:
            printf("interface\n");
            break;
        case DT_STRUCT:
            printf("struct\n");
            break;
        case DT_DATA:
            printf("data\n");
            break;
        case DT_FN:
            printf("fn\n");
            break;
        case DT_PTR:
            printf("ptr\n");
            break;
        case DT_REFERENCE:
            printf("ref\n");
            break;
        case DT_TYPE_JOIN:
            printf("join\n");
            break;
        case DT_TYPE_UNION:
            printf("union\n");
            break;
        case DT_UNRESOLVED:
            printf("unresolved\n");
            break;
    }
}
/**
* Building AST Data types
*/

ArrayType* ast_type_makeArray() {
    ALLOC(array, ArrayType);
    array->len = 0;
    return array;
}

EnumType* ast_type_makeEnum(){
    ALLOC(enu, EnumType);
    map_init(&enu->enums);
    vec_init(&enu->enumNames);

    return enu;
}

PtrType * ast_type_makePtr() {
    ALLOC(ptr, PtrType);
    ptr->target = NULL;

    return ptr;
}

ReferenceType* ast_type_makeReference() {
    ALLOC(ref, ReferenceType);
    ref->ref = NULL;

    return ref;
}

JoinType* ast_type_makeJoin() {
    ALLOC(join, JoinType);
    map_init(&join->joins);

    return join;
}

UnionType* ast_type_makeUnion() {
    ALLOC(uni, UnionType);
    map_init(&uni->unions);

    return uni;
}

DataDataType* ast_type_makeData() {
    ALLOC(data, DataDataType);
    map_init(&data->constructors);
    vec_init(&data->constructorNames);

    return data;
}

InterfaceType* ast_type_makeInterface() {
    ALLOC(interface, InterfaceType);
    map_init(&interface->methods);
    vec_init(&interface->methodNames);

    return interface;
}

ClassType* ast_type_makeClass() {
    ALLOC(class_, ClassType);
    map_init(&class_->methods);
    map_init(&class_->attributes);
    vec_init(&class_->attributeNames);
    vec_init(&class_->methodNames);

    return class_;
}

FnType* ast_type_makeFn() {
    ALLOC(fn, FnType);
    map_init(&fn->args);
    vec_init(&fn->argsNames);
    fn->returnType = NULL;

    return fn;
}

DataType* ast_type_makeType() {
    ALLOC(type, DataType);
    type->name = NULL;
    type->isGeneric = 0;
    type->kind = DT_UNRESOLVED;
    type ->classType = NULL;

    return type;
}

#undef ALLOC