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
struct FnHeader;
struct ClassMethod;
struct ClassAttribute;
struct StructAttribute;
struct FnArgument;
struct GenericParam;

typedef map_t(uint32_t) u32_map_t;
typedef map_t(struct DataType*) dtype_map_t;
typedef map_t(struct DataConstructor*) dataconstructor_map_t;
typedef map_t(struct FnHeader*) interfacemethod_map_t;
typedef map_t(struct ClassMethod*) classmethod_map_t;
typedef map_t(struct ClassAttribute*) classattribute_map_t;
typedef map_t(struct StructAttribute*) structattribute_map_t;
typedef map_t(struct FnArgument*) fnargument_map_t;
typedef map_t(struct VariantConstructorArgument*) variantconstructorarg_map_t;
typedef map_t(struct VariantConstructor*) variantconstructor_map_t;

typedef vec_t(struct GenericParam*) genericparam_vec_t;
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
    DT_ENUM, DT_VARIANT,
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
    struct DataType* left;
    struct DataType* right;
}JoinType;
JoinType* ast_type_makeJoin();

typedef struct UnionType {
    struct DataType* left;
    struct DataType* right;
}UnionType;
UnionType* ast_type_makeUnion();

typedef struct VariantType {
    variantconstructor_map_t constructors;
    vec_str_t  constructorNames;
}VariantType;
VariantType* ast_type_makeVariant();

typedef struct InterfaceType {
    interfacemethod_map_t methods;
    vec_str_t  methodNames;
    dtype_vec_t extends;
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
    vec_str_t argNames;
    struct DataType* returnType;
}FnType;
FnType* ast_type_makeFn();

typedef struct StructType {
    structattribute_map_t attributes;
    // important for layout management
    vec_str_t attributeNames;
    dtype_vec_t extends;
}StructType;
StructType* ast_type_makeStruct();

typedef struct GenericParam {
    uint8_t isGeneric;
    // if is generic, we check the name
    // else we check the type
    char* name;
    struct DataType* type;
    struct DataType* constraint; // i.e must be an instance of something
}GenericParam;
GenericParam* ast_make_genericParam();

typedef struct DataType {
    char* name;
    DataTypeKind kind;
    uint8_t hasGeneric;
    uint8_t isNullable;

    genericparam_vec_t genericParams;

    union {
        ClassType * classType;
        InterfaceType * interfaceType;
        StructType* structType;
        VariantType * variantType;
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

typedef struct VariantConstructorArgument {
    char* name;
    DataType * type;
}VariantConstructorArgument;
VariantConstructorArgument * ast_type_makeVariantConstructorArgument();

typedef struct VariantConstructor {
    char* name;
    vec_str_t argNames;
    variantconstructorarg_map_t args;
}VariantConstructor;
VariantConstructor* ast_type_makeVariantConstructor();

typedef struct StructAttribute {
    char* name;
    DataType * type;
}StructAttribute;
StructAttribute * ast_type_makeStructAttribute();

typedef struct ClassAttribute {
    char* name;
    DataType * type;
}ClassAttribute;
ClassAttribute * ast_type_makeClassAttribute();

typedef struct FnHeader {
    char* name;
    FnType* type;
    uint8_t isGeneric;
    vec_str_t genericNames;
    u32_map_t generics;
} FnHeader;
FnHeader*  ast_makeFnHeader();

typedef struct  ClassMethod {
    struct FnHeader* header;
} ClassMethod;



/**
 * A generic descriptors
 */
typedef struct GenericEntity {
    char* name;
    vec_str_t supers;
}GenericEntity;


typedef struct DataConstructor {
    char* name;
    vec_str_t argNames;

}DataConstructor;


typedef struct ImportStmt {
    // combines both source and test
    PackageID* path;
    uint8_t hasAlias;
    char* alias;
}ImportStmt;

typedef struct FnArgument {
    char* name;
    uint8_t isMutable;
    DataType * type;
}FnArgument;
FnArgument * ast_type_makeFnArgument();

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
