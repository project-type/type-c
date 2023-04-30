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

// returns a string representation of the type
char* ast_strigifyType(DataType* type){
    // create base string
    char* str = strdup("");
    if(type->name != NULL){
        str = realloc(str, strlen(str) + strlen(type->name) + 1);
        strcat(str, type->name);
    }
    // we check if it has generics
    if(type->isGeneric){
        // we add the generics to the string
        int i; char * val;
        // first we add a "<" to the string
        str = realloc(str, strlen(str) + 1);
        strcat(str, "<");
        // then we add the generics
        vec_foreach(&type->genericNames, val, i) {
            str = realloc(str, strlen(str) + strlen(val) + 1);
            strcat(str, val);
        }
        // we close >
        str = realloc(str, strlen(str) + 1);
        strcat(str, ">");
    }
    // we check its kind
    switch (type->kind) {
        case DT_UNRESOLVED:
            str = realloc(str, strlen(str) + strlen("unresolved") + 1);
            strcat(str, "unresolved");
            break;
        case DT_I8:
            str = realloc(str, strlen(str) + strlen("i8") + 1);
            strcat(str, "i8");
            break;
        case DT_I16:
            str = realloc(str, strlen(str) + strlen("i16") + 1);
            strcat(str, "i16");
            break;
        case DT_I32:
            str = realloc(str, strlen(str) + strlen("i32") + 1);
            strcat(str, "i32");
            break;

        case DT_I64:
            str = realloc(str, strlen(str) + strlen("i64") + 1);
            strcat(str, "i64");
            break;
        case DT_U8:
            str = realloc(str, strlen(str) + strlen("u8") + 1);
            strcat(str, "u8");
            break;
        case DT_U16:
            str = realloc(str, strlen(str) + strlen("u16") + 1);
            strcat(str, "u16");
            break;
        case DT_U32:
            str = realloc(str, strlen(str) + strlen("u32") + 1);
            strcat(str, "u32");
            break;
        case DT_U64:
            str = realloc(str, strlen(str) + strlen("u64") + 1);
            strcat(str, "u64");
            break;
        case DT_F32:
            str = realloc(str, strlen(str) + strlen("f32") + 1);
            strcat(str, "f32");
            break;
        case DT_F64:
            str = realloc(str, strlen(str) + strlen("f64") + 1);
            strcat(str, "f64");
            break;
        case DT_BOOL:
            str = realloc(str, strlen(str) + strlen("bool") + 1);
            strcat(str, "bool");
            break;
        case DT_CHAR:
            str = realloc(str, strlen(str) + strlen("char") + 1);
            strcat(str, "char");
            break;
        case DT_STRING:
            str = realloc(str, strlen(str) + strlen("string") + 1);
            strcat(str, "string");
            break;
        case DT_ARRAY:
            // we print it in the form of array<type, size>
            str = realloc(str, strlen(str) + strlen("array") + 1);
            strcat(str, "array");
            str = realloc(str, strlen(str) + strlen("<") + 1);
            strcat(str, "<");
            char* arrayType = ast_strigifyType(type->arrayType->arrayOf);
            str = realloc(str, strlen(str) + strlen(arrayType) + 1);
            strcat(str, arrayType);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            char* arraySize = malloc(20);
            sprintf(arraySize, "%llu", type->arrayType->len);
            str = realloc(str, strlen(str) + strlen(arraySize) + 1);
            strcat(str, arraySize);
            str = realloc(str, strlen(str) + strlen(">") + 1);
            strcat(str, ">");

            break;
        case DT_ENUM:
            str = realloc(str, strlen(str) + strlen("enum") + 1);
            strcat(str, "enum");
            // we add all fields in the form of enum{field1, field2, field3}
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->enumType->enumNames, val, i) {
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';

            break;
        case DT_REFERENCE:
            // we check if package is null, if its not we add it to the string as <p1.p2.p3>
            if(type->refType->pkg != NULL){
                str = realloc(str, strlen(str) + strlen("<") + 1);
                strcat(str, "<");
                // we loop
                int i; char * val;
                vec_foreach(&type->refType->pkg->ids, val, i) {
                    str = realloc(str, strlen(str) + strlen(val) + 1);
                    strcat(str, val);
                    str = realloc(str, strlen(str) + strlen(".") + 1);
                    strcat(str, ".");
                }
                // we replace the last . with >
                str[strlen(str) - 1] = '>';
            }
            else {
                // we throw an error if the ref is nul
                if(type->refType->ref == NULL){
                    printf("Error: reference type is null\n");
                    exit(1);
                }

                // we add the name of the reference type, or "anynomous" if the name is null
                if(type->refType->ref->name == NULL){
                    str = realloc(str, strlen(str) + strlen("anynomous") + 1);
                    strcat(str, "anynomous");
                }
                else {
                    str = realloc(str, strlen(str) + strlen(type->refType->ref->name) + 1);
                    strcat(str, type->refType->ref->name);
                }
            }
            break;
        case DT_TYPE_UNION: {
            // we print union in the form of union{type1, type2, type3}
            str = realloc(str, strlen(str) + strlen("union") + 1);
            strcat(str, "union");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i;
            char *val;
            vec_foreach(&type->unionType->unions, val, i) {
                char *unionType = ast_strigifyType(val);
                str = realloc(str, strlen(str) + strlen(unionType) + 1);
                strcat(str, unionType);
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_TYPE_JOIN: {
            // same as we did to union but with join
            str = realloc(str, strlen(str) + strlen("join") + 1);
            strcat(str, "join");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i;
            char *val;
            vec_foreach(&type->joinType->joins, val, i) {
                char *joinType = ast_strigifyType(val);
                str = realloc(str, strlen(str) + strlen(joinType) + 1);
                strcat(str, joinType);
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;

        }
        default:
            break;
    }

    return str;
}

void ast_debug_Type(DataType* type){
    printf("type %s\n", type->name);
    if(type->isGeneric){
        int i; char * val;
        printf("Generics: ");
        vec_foreach(&type->genericNames, val, i) {
            printf(" %s ", val);
        }
        printf("\n");
    }

    switch(type->kind) {
        case DT_ENUM:
            printf("\t is ENUM\n");
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
        case DT_REFERENCE:{
            printf("ref<");
            int i; char * val;
            vec_foreach(&type->refType->pkg->ids, val, i) {
                printf(" %s ", val);
            }
            printf("\n");
            break;
        }
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
    vec_init(&join->joins);

    return join;
}

UnionType* ast_type_makeUnion() {
    ALLOC(uni, UnionType);
    vec_init(&uni->unions);

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

    vec_init(&type->genericNames);
    map_init(&type->generics);

    return type;
}

#undef ALLOC