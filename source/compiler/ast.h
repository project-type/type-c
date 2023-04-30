//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_AST_H
#define TYPE_C_AST_H

#include "../utils/vec.h"
#include "../utils/map.h"
#include "tokens.h"

/**
 * Basic types
 */

typedef struct PackageID {
    vec_str_t ids;
}PackageID;

/**
 * Forward declarations
 */
struct DataType;
struct DataConstructor;
struct InterfaceMethod;
struct ClassMethod;
struct ClassAttribute;
struct FnArgument;

typedef map_t(uint32_t) u32_map_t;
typedef map_t(struct DataType*) dtype_map_t;
typedef map_t(struct DataConstructor*) dataconstructor_map_t;
typedef map_t(struct InterfaceMethod*) interfacemethod_map_t;
typedef map_t(struct ClassMethod*) classmethod_map_t;
typedef map_t(struct ClassAttribute*) classattribute_map_t;
typedef map_t(struct FnArgument*) fnargument_map_t;

typedef vec_t(struct DataType*) dtype_vec_t;
/**
 * Enums of all possible type categories
 */
typedef enum DataTypeKind {
    DT_I8, DT_I16, DT_I32, DT_I64,
    DT_U8, DT_U16, DT_U32, DT_U64,
    DT_F32, DT_F64,
    DT_BOOL,
    DT_STRING,
    DT_CHAR,
    DT_CLASS, DT_INTERFACE,
    DT_STRUCT,
    DT_ENUM, DT_DATA,
    DT_ARRAY,
    DT_FN,
    DT_PTR,
    DT_REFERENCE,
    DT_TYPE_JOIN,
    DT_TYPE_UNION,
    DT_UNRESOLVED // means parser didn't see this type yet so its on hold.
}DataTypeKind;

/** Array data type structure e.g u32[] */
typedef struct ArrayType {
    uint64_t len;
    struct DataType* arrayOf;
}ArrayType;
ArrayType* ast_type_makeArray();

/** enum */
typedef struct EnumType {
    u32_map_t enums;
    vec_str_t enumNames;
}EnumType;
EnumType* ast_type_makeEnum();

typedef struct PtrType {
    struct DataType* target;
}PtrType;
PtrType * ast_type_makePtr();

typedef struct ReferenceType {
    PackageID* pkg;
    struct DataType* ref;
}ReferenceType;
ReferenceType* ast_type_makeReference();

typedef struct JoinType {
    dtype_vec_t joins;
}JoinType;
JoinType* ast_type_makeJoin();

typedef struct UnionType {
    dtype_vec_t unions;
}UnionType;
UnionType* ast_type_makeUnion();

typedef struct DataDataType {
    dataconstructor_map_t constructors;
    vec_str_t  constructorNames;
}DataDataType;
DataDataType* ast_type_makeData();

typedef struct InterfaceType {
    interfacemethod_map_t methods;
    vec_str_t  methodNames;
}InterfaceType;
InterfaceType* ast_type_makeInterface();

typedef struct ClassType {
    classattribute_map_t attributes;
    classmethod_map_t methods;

    // important for layout management
    vec_str_t attributeNames;
    vec_str_t methodNames;
}ClassType;
ClassType* ast_type_makeClass();

typedef struct FnType {
    fnargument_map_t args;
    vec_str_t argsNames;
    struct DataType* returnType;
}FnType;
FnType* ast_type_makeFn();

typedef struct DataType {
    char* name;
    DataTypeKind kind;
    uint8_t isGeneric;

    vec_str_t genericNames;
    u32_map_t generics;

    union {
        ClassType * classType;
        InterfaceType * interfaceType;
        DataDataType * structType;
        EnumType * enumType;
        UnionType * unionType;
        JoinType * joinType;
        ArrayType * arrayType;
        FnType * fnType;
        PtrType* ptrType;
        ReferenceType* refType;
    };
}DataType;
DataType* ast_type_makeType();

typedef struct  ClassMethod {
    char* name;
    // TODO: add method decl
} ClassMethod;

typedef struct ClassAttribute {
    char* name;
    // TODO: add attribute types
}ClassAttribute;

typedef struct InterfaceMethod {
    char* name;
    // TODO: add interface decl
}InterfaceMethod;



/**
 * A generic descriptors
 */
typedef struct GenericEntity {
    char* name;
    vec_str_t supers;
}GenericEntity;


typedef struct DataConstructor {
    vec_str_t args;
}DataConstructor;


typedef struct ImportStmt {
    // combines both source and test
    PackageID* path;
    uint8_t hasAlias;
    char* alias;
}ImportStmt;

typedef struct FnArgument {
    char* name;
    uint8_t is_mut;
    DataType * type;
}FnArgument;

typedef vec_t(ImportStmt*) import_stmt_vec;


typedef enum ASTNodeType{
    AST_PROGRAM,
}ASTNodeType;

typedef struct ASTScope {
    void* variables;
    void* functions;
    dtype_map_t dataTypes;
    void* statements;
    void* externDeclarations;
    void* expressions;
} ASTScope;

typedef struct ASTProgramNode {
    import_stmt_vec importStatements;
}ASTProgramNode;

typedef struct ASTNode {
    ASTScope scope;
    ASTNodeType type;
    union {
        ASTProgramNode* programNode;
    };
}ASTNode;


ASTNode * ast_makeProgramNode();
PackageID* ast_makePackageID();
ImportStmt* ast_makeImportStmt(PackageID* source, PackageID* target, uint8_t hasAlias, char* alias);

// debugging
char* ast_stringifyType(DataType* type);
char* ast_stringifyImport(ASTProgramNode* node, uint32_t index);
#endif //TYPE_C_AST_H
