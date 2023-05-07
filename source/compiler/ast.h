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
struct UnresolvedType;
struct LetExprDecl;

struct Expr;
struct CaseExpr;

struct Statement;
struct BlockStatement;
struct CaseStatement;

/* Data types */
typedef map_t(uint32_t) u32_map_t;
typedef map_t(struct DataType*) map_dtype_t;
typedef map_t(struct DataConstructor*) map_dataconstructor_t;
typedef map_t(struct FnHeader*) map_interfacemethod_t;
typedef map_t(struct ClassMethod*) map_classmethod_t;
typedef map_t(struct ClassAttribute*) map_classattribute_t;
typedef map_t(struct StructAttribute*) map_structattribute_t;
typedef map_t(struct FnArgument*) map_fnargument_t;
typedef map_t(struct VariantConstructorArgument*) map_variantconstructorarg_t;
typedef map_t(struct VariantConstructor*) map_variantconstructor_t;
typedef map_t(struct Expr*) mat_expr_t;

typedef vec_t(struct GenericParam*) vec_genericparam_t;
typedef vec_t(struct DataType*) vec_dtype_t;
typedef vec_t(struct UnresolvedType) vec_unresolvedtype_t;

/* Expressions */
typedef vec_t(struct Expr*) vec_expr_t;
typedef vec_t(struct LetExprDecl*) vec_letexprlist_t;
typedef vec_t(struct CaseExpr*) vec_caseexpr_t;
typedef vec_t(struct Statement*) vec_statement_t;
typedef vec_t(struct BlockStatement*) vec_blockstatement_t;
typedef vec_t(struct CaseStatement*) vec_casestatement_t;

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
    map_variantconstructor_t constructors;
    vec_str_t  constructorNames;
}VariantType;
VariantType* ast_type_makeVariant();

typedef struct InterfaceType {
    map_interfacemethod_t methods;
    vec_str_t  methodNames;
    vec_dtype_t extends;
}InterfaceType;
InterfaceType* ast_type_makeInterface();

typedef struct ClassType {
    map_classattribute_t attributes;
    map_classmethod_t methods;

    // important for layout management
    vec_str_t attributeNames;
    vec_str_t methodNames;
}ClassType;
ClassType* ast_type_makeClass();

typedef struct FnType {
    map_fnargument_t args;
    vec_str_t argNames;
    struct DataType* returnType;
}FnType;
FnType* ast_type_makeFn();

typedef struct StructType {
    map_structattribute_t attributes;
    // important for layout management
    vec_str_t attributeNames;
    vec_dtype_t extends;
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

    vec_genericparam_t genericParams;

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
    map_variantconstructorarg_t args;
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
    uint8_t isSafe;
    void* variables;
    void* functions;
    map_dtype_t dataTypes;
    void* statements;
    void* externDeclarations;
    struct ASTScope* parentScope;
} ASTScope;
ASTScope * ast_scope_makeScope(ASTScope* parentScope);

// this is used to store unresolved types, meaning types whose dependencies hasn't been
// found yet. A program will only run after all dependencies are resolved.
typedef struct UnresolvedType{
    DataType* typeRef;
    ASTScope* scope;
}UnresolvedType;

typedef struct ASTProgramNode {
    import_stmt_vec importStatements;
    vec_unresolvedtype_t unresolvedTypes;
}ASTProgramNode;

typedef struct ASTNode {
    ASTScope* scope;
    ASTNodeType type;
    union {
        ASTProgramNode* programNode;
    };
}ASTNode;

typedef enum BinaryExprType {
    BET_ADD,
    BET_SUB,
    BET_MUL,
    BET_DIV,
    BET_MOD,
    BET_AND,
    BET_OR,
    BET_BIT_XOR,
    BET_BIT_AND,
    BET_BIT_OR,
    BET_LSHIFT,
    BET_RSHIFT,
    BET_EQ,
    BET_NEQ,
    BET_LT,
    BET_LTE,
    BET_GT,
    BET_GTE,
    BET_ASSIGN,
    BET_ADD_ASSIGN,
    BET_SUB_ASSIGN,
    BET_MUL_ASSIGN,
    BET_DIV_ASSIGN
}BinaryExprType;

typedef enum UnaryExprType {
    UET_NOT,
    UET_BIT_NOT,
    UET_NEG,
    UET_PRE_INC,
    UET_PRE_DEC,
    UET_POST_INC,
    UET_POST_DEC,
    UET_DEREF,
    UET_ADDRESS_OF,
    UET_DENULL,
}UnaryExprType;


typedef enum LiteralType {
    LT_STRING,
    LT_INTEGER,
    LT_BINARY_INT,
    LT_OCTAL_INT,
    LT_HEX_INT,
    LT_DOUBLE,
    LT_FLOAT,
    LT_BOOLEAN,
    LT_CHARACTER
}LiteralType;

// 3.14
typedef struct LiteralExpr {
    LiteralType type;
    char* value;
}LiteralExpr;
LiteralExpr* ast_expr_makeLiteralExpr(LiteralType type);

// x
typedef struct ElementExpr {
    char* name;
}ElementExpr;
ElementExpr* ast_expr_makeElementExpr(char* name);

// x++
typedef struct UnaryExpr {
    UnaryExprType type;
    struct Expr *uhs;
}UnaryExpr;
UnaryExpr* ast_expr_makeUnaryExpr(UnaryExprType type, struct Expr *expr);

// x + y
typedef struct BinaryExpr {
    BinaryExprType type;
    struct Expr *lhs;
    struct Expr *rhs;
}BinaryExpr;
BinaryExpr* ast_expr_makeBinaryExpr(BinaryExprType type, struct Expr *left, struct Expr *right);

// new x()
typedef struct NewExpr {
    DataType *type;
    vec_expr_t args;
}NewExpr;
NewExpr* ast_expr_makeNewExpr(DataType *type);

// x()
typedef struct CallExpr {
    struct Expr *lhs;
    vec_expr_t args;
    uint8_t hasGenerics;
    vec_dtype_t generics;
}CallExpr;
CallExpr* ast_expr_makeCallExpr(struct Expr *lhs);

// x.y
typedef struct MemberAccessExpr {
    struct Expr *lhs, *rhs;
}MemberAccessExpr;
MemberAccessExpr* ast_expr_makeMemberAccessExpr(struct Expr *lhs, struct Expr *rhs);

// x[10]
typedef struct IndexAccessExpr {
    struct Expr *expr;
    vec_expr_t indexes;
}IndexAccessExpr;
IndexAccessExpr* ast_expr_makeIndexAccessExpr(struct Expr *expr);

// (10 as u32)
typedef struct CastExpr {
    DataType *type;
    struct Expr *expr;
}CastExpr;
CastExpr* ast_expr_makeCastExpr(DataType *type, struct Expr *expr);

typedef struct InstanceCheckExpr {
    struct Expr *expr;
    DataType *type;
}InstanceCheckExpr;
InstanceCheckExpr* ast_expr_makeInstanceCheckExpr(DataType *type, struct Expr *expr);


// if condition else (if condition else ( ... ))
typedef struct IfElseExpr {
    struct Expr *condition; //
    struct Expr *ifExpr;
    struct Expr *elseExpr;
}IfElseExpr;
IfElseExpr* ast_expr_makeIfElseExpr();

// case 1 => 1
typedef struct CaseExpr {
    struct Expr *condition;
    struct Expr *expr;
}CaseExpr;
CaseExpr * ast_expr_makeCaseExpr(struct Expr *condition, struct Expr *expr);

// match x { 1 => 1, 2 => 2, _ => 3 }
typedef struct MatchExpr {
    struct Expr *expr;
    vec_caseexpr_t cases;
    CaseExpr* elseCase;
}MatchExpr;
MatchExpr* ast_expr_makeMatchExpr(struct Expr *expr);

typedef enum LetInitializerType {
    LIT_NONE,
    LIT_STRUCT_DECONSTRUCTION,
    LIT_ARRAY_DECONSTRUCTION
}LetInitializerType;

typedef struct LetExprDecl {
    LetInitializerType initializerType;
    vec_str_t variableNames;
    map_fnargument_t variables;
    struct Expr *initializer;
}LetExprDecl;
LetExprDecl* ast_expr_makeLetExprDecl();

// let x = 10, y = 3 in x + 10
typedef struct LetExpr {
    vec_letexprlist_t letList;
    struct Expr *inExpr;
    ASTScope* scope;
}LetExpr;
LetExpr* ast_expr_makeLetExpr(ASTScope* parentScope);

typedef struct ArrayConstructionExpr {
    DataType *type;
    vec_expr_t args;
}ArrayConstructionExpr;
ArrayConstructionExpr * ast_expr_makeArrayConstructionExpr();

typedef struct NamedStructConstructionExpr {
    DataType *type;
    vec_str_t argNames;
    mat_expr_t args;
}NamedStructConstructionExpr;
NamedStructConstructionExpr * ast_expr_makeNamedStructConstructionExpr();

typedef struct UnnamedStructConstructionExpr {
    DataType *type;
    vec_expr_t args;
}UnnamedStructConstructionExpr;
UnnamedStructConstructionExpr * ast_expr_makeUnnamedStructConstructionExpr();

typedef enum FnBodyType {
    FBT_EXPR,
    FBT_BLOCK
}FnBodyType;

typedef struct LambdaExpr {
    FnHeader * header;

    ASTScope* scope;
    FnBodyType bodyType;
    union {
        struct Expr *expr;
        struct BlockStatement *block;
    };
}LambdaExpr;
LambdaExpr* ast_expr_makeLambdaExpr(ASTScope* parentScope);

typedef struct UnsafeExpr {
    struct Expr *expr;
    ASTScope *scope;
}UnsafeExpr;
UnsafeExpr* ast_expr_makeUnsafeExpr(ASTScope* parentScope);

typedef enum ExpressionType {
    ET_LITERAL,
    ET_ELEMENT,
    ET_ARRAY_CONSTRUCTION,
    ET_NAMED_STRUCT_CONSTRUCTION,
    ET_UNNAMED_STRUCT_CONSTRUCTION,
    ET_NEW,
    ET_CALL,
    ET_MEMBER_ACCESS,
    ET_INDEX_ACCESS,
    ET_CAST,
    ET_INSTANCE_CHECK,
    ET_UNARY,
    ET_BINARY,
    ET_IF_ELSE,
    ET_MATCH,
    ET_LET,
    ET_LAMBDA,
    ET_UNSAFE
}ExpressionType;

typedef struct Expr {
    ExpressionType type;
    DataType* dataType;
    union {
        ArrayConstructionExpr * arrayConstructionExpr;
        NamedStructConstructionExpr * namedStructConstructionExpr;
        UnnamedStructConstructionExpr * unnamedStructConstructionExpr;
        LiteralExpr* literalExpr;
        ElementExpr* elementExpr;
        LetExpr * letExpr;
        NewExpr* newExpr;
        CallExpr* callExpr;
        MemberAccessExpr* memberAccessExpr;
        IndexAccessExpr* indexAccessExpr;
        CastExpr* castExpr;
        InstanceCheckExpr * instanceCheckExpr;
        UnaryExpr* unaryExpr;
        BinaryExpr* binaryExpr;
        IfElseExpr* ifElseExpr;
        MatchExpr* matchExpr;
        LambdaExpr* lambdaExpr;
        UnsafeExpr* unsafeExpr;
    };
}Expr;
Expr* ast_expr_makeExpr(ExpressionType type);

typedef struct BlockStatement {
    vec_statement_t stmts;
    ASTScope *scope;
}BlockStatement;
BlockStatement* ast_stmt_makeBlockStatement(ASTScope* parentScope);

typedef struct VarDeclStatement {
    vec_letexprlist_t letList;
    ASTScope* scope;
}VarDeclStatement;
VarDeclStatement* ast_stmt_makeVarDeclStatement(ASTScope* parentScope);

typedef struct FnDeclStatement {
    FnHeader * header;
    FnBodyType bodyType;
    union {
        struct Expr *expr;
        struct Statement *block;
    };
    ASTScope * scope;

    uint8_t hasGeneric;
    vec_genericparam_t genericParams;
}FnDeclStatement;
FnDeclStatement* ast_stmt_makeFnDeclStatement(ASTScope* parentScope);

typedef struct IfChainStatement {
    vec_expr_t conditions;
    vec_statement_t blocks;
    struct Statement *elseBlock;
}IfChainStatement;
IfChainStatement* ast_stmt_makeIfChainStatement();

typedef struct CaseStatement {
    struct Expr *condition;
    struct Statement *block;
}CaseStatement;
CaseStatement* ast_stmt_makeCaseStatement();

typedef struct MatchStatement {
    struct Expr *expr;
    vec_casestatement_t cases;
    struct Statement * elseBlock;
}MatchStatement;
MatchStatement* ast_stmt_makeMatchStatement();

typedef struct WhileStatement {
    char* label;
    struct Expr *condition;
    struct Statement *block;
}WhileStatement;
WhileStatement* ast_stmt_makeWhileStatement();

typedef struct DoWhileStatement {
    char* label;
    struct Expr *condition;
    struct Statement *block;
}DoWhileStatement;
DoWhileStatement* ast_stmt_makeDoWhileStatement();

typedef struct ForStatement {
    char* label;
    struct Statement *initializer;
    struct Expr *condition;
    vec_expr_t increments;
    struct Statement *block;
    ASTScope * scope;
}ForStatement;
ForStatement* ast_stmt_makeForStatement(ASTScope* parentScope);

typedef struct ForEachStatement{
    char* variableContentName;
    char* variableIndexName;
    struct Expr *iterable;
    struct Statement *block;
    ASTScope * scope;
}ForEachStatement;
ForEachStatement* ast_stmt_makeForEachStatement(ASTScope* parentScope);

typedef struct ContinueStatement {
    char *label;
}ContinueStatement;
ContinueStatement* ast_stmt_makeContinueStatement();

typedef struct BreakStatement {
    char *label;
}BreakStatement;
BreakStatement* ast_stmt_makeBreakStatement();

typedef struct ReturnStatement {
    struct Expr *expr;
}ReturnStatement;
ReturnStatement* ast_stmt_makeReturnStatement();

typedef struct UnsafeStatement {
    struct Statement *block;
}UnsafeStatement;
UnsafeStatement* ast_stmt_makeUnsafeStatement();

typedef struct ExprStatement {
    struct Expr *expr;
}ExprStatement;
ExprStatement* ast_stmt_makeExprStatement();

typedef enum StatementType {
    ST_EXPR,
    ST_VAR_DECL,
    ST_FN_DECL,
    ST_BLOCK,

    // conditional
    ST_IF_CHAIN,
    ST_MATCH,

    // iterative
    ST_WHILE,
    ST_DO_WHILE,
    ST_FOR,
    ST_FOREACH,

    // keywords
    ST_CONTINUE,
    ST_RETURN,
    //ST_DELETE,
    ST_BREAK,

    // BLOCK
    ST_UNSAFE,
}StatementType;

typedef struct Statement {
    StatementType type;
    union {
        ExprStatement * expr;
        VarDeclStatement * varDecl;
        FnDeclStatement * fnDecl;
        IfChainStatement * ifChain;
        MatchStatement * match;
        WhileStatement * whileLoop;
        ForStatement * forLoop;
        ForEachStatement * foreachLoop;
        DoWhileStatement * doWhileLoop;
        ContinueStatement * continueStmt;
        ReturnStatement * returnStmt;
        //DeleteStatement* deleteStmt;
        BreakStatement * breakStmt;
        BlockStatement * blockStmt;
        UnsafeStatement * unsafeStmt;
        //void* withStmt;
    };
}Statement;
Statement* ast_stmt_makeStatement(StatementType type);

ASTNode * ast_makeProgramNode();
PackageID* ast_makePackageID();
ImportStmt* ast_makeImportStmt(PackageID* source, PackageID* target, uint8_t hasAlias, char* alias);

// debugging
#endif //TYPE_C_AST_H
