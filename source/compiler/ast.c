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
    vec_init(&program->unresolvedTypes);

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->programNode = program;
    map_init(&node->scope.dataTypes);
    node->scope.parentScope = NULL;

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
    join->left = NULL;
    join->right = NULL;

    return join;
}

UnionType* ast_type_makeUnion() {
    ALLOC(uni, UnionType);
    uni->left = NULL;
    uni->right = NULL;

    return uni;
}

VariantType* ast_type_makeVariant() {
    ALLOC(data, VariantType);
    map_init(&data->constructors);
    vec_init(&data->constructorNames);

    return data;
}

InterfaceType* ast_type_makeInterface() {
    ALLOC(interface, InterfaceType);
    map_init(&interface->methods);
    vec_init(&interface->methodNames);
    vec_init(&interface->extends);

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
    vec_init(&fn->argNames);
    fn->returnType = NULL;

    return fn;
}

StructType* ast_type_makeStruct() {
    ALLOC(struct_, StructType);
    map_init(&struct_->attributes);
    vec_init(&struct_->attributeNames);
    vec_init(&struct_->extends);

    return struct_;
}

DataType* ast_type_makeType() {
    ALLOC(type, DataType);
    type->name = NULL;
    type->hasGeneric = 0;
    type->isNullable = 0;
    type->kind = DT_UNRESOLVED;
    type->classType = NULL;
    vec_init(&type->genericParams);

    return type;
}

StructAttribute * ast_type_makeStructAttribute(){
    ALLOC(attribute, StructAttribute);
    attribute->type = NULL;
    attribute->name = NULL;

    return attribute;
}

ClassAttribute * ast_type_makeClassAttribute(){
    ALLOC(attribute, ClassAttribute);
    attribute->type = NULL;
    attribute->name = NULL;

    return attribute;
}

VariantConstructorArgument * ast_type_makeVariantConstructorArgument() {
    ALLOC(argument, VariantConstructorArgument);
    argument->type = NULL;
    argument->name = NULL;

    return argument;
}

VariantConstructor* ast_type_makeVariantConstructor(){
    ALLOC(constructor, VariantConstructor);
    map_init(&constructor->args);
    vec_init(&constructor->argNames);

    return constructor;
}

FnHeader*  ast_makeFnHeader(){
    ALLOC(header, FnHeader);
    header->name = NULL;
    header->type = NULL;
    header->isGeneric = 0;
    // init vec and map
    vec_init(&header->genericNames);
    map_init(&header->generics);

    return header;
}

FnArgument * ast_type_makeFnArgument(){
    ALLOC(argument, FnArgument);
    argument->type = NULL;
    argument->name = NULL;
    argument->isMutable = 0;

    return argument;
}

GenericParam* ast_make_genericParam() {
    ALLOC(param, GenericParam);
    param->name = NULL;
    param->type = NULL;
    param->isGeneric = 0;
    param->constraint = NULL;

    return param;
}

ElementExpr* ast_expr_makeElementExpr(char* name){
    ALLOC(element, ElementExpr);
    element->name = strdup(name);

    return element;
}

UnaryExpr* ast_expr_makeUnaryExpr(UnaryExprType type, struct Expr *expr){
    ALLOC(unary, UnaryExpr);
    unary->type = type;
    unary->uhs = expr;

    return unary;
}

BinaryExpr* ast_expr_makeBinaryExpr(BinaryExprType type, struct Expr *left, struct Expr *right){
    ALLOC(binary, BinaryExpr);
    binary->type = type;
    binary->lhs = left;
    binary->rhs = right;

    return binary;
}

NewExpr* ast_expr_makeNewExpr(DataType *type){
    ALLOC(new, NewExpr);
    new->type = type;
    vec_init(&new->args);

    return new;
}

CallExpr* ast_expr_makeCallExpr(struct Expr *lhs){
    ALLOC(call, CallExpr);
    call->lhs = lhs;
    vec_init(&call->args);

    return call;
}

MemberAccessExpr* ast_expr_makeMemberAccessExpr(struct Expr *lhs, struct Expr *rhs){
    ALLOC(memberAccess, MemberAccessExpr);
    memberAccess->lhs = lhs;
    memberAccess->rhs = rhs;

    return memberAccess;
}

IndexAccessExpr* ast_expr_makeIndexAccessExpr(struct Expr *expr){
    ALLOC(indexAccess, IndexAccessExpr);
    indexAccess->expr = expr;
    vec_init(&indexAccess->indexes);

    return indexAccess;
}

CastExpr* ast_expr_makeCastExpr(DataType *type, struct Expr *expr){
    ALLOC(cast, CastExpr);
    cast->type = type;
    cast->expr = expr;

    return cast;
}

InstanceCheckExpr* ast_expr_makeInstanceCheckExpr(DataType *type, struct Expr *expr){
    ALLOC(instanceCheck, InstanceCheckExpr);
    instanceCheck->expr = expr;
    instanceCheck->type = type;

    return instanceCheck;
}

IfElseExpr* ast_expr_makeIfElseExpr(struct Expr *condition, struct Expr *ifExpr, struct Expr *elseExpr) {
    ALLOC(ifElse, IfElseExpr);
    ifElse->condition = condition;
    ifElse->ifExpr = ifExpr;
    ifElse->elseExpr = elseExpr;

    return ifElse;
}

CaseExpr * ast_expr_makeCaseExpr(struct Expr *condition, struct Expr *expr){
    ALLOC(caseExpr, CaseExpr);
    caseExpr->condition = condition;
    caseExpr->expr = expr;

    return caseExpr;
}

MatchExpr* ast_expr_makeMatchExpr(struct Expr *expr) {
    ALLOC(match, MatchExpr);
    match->expr = expr;
    vec_init(&match->cases);

    return match;
}

LetExpr* ast_expr_makeLetExpr(ASTScope* parentScope){
    ALLOC(let, LetExpr);
    vec_init(&let->letList);
    let->inExpr = NULL;
    vec_init(&let->scope.dataTypes);
    let->scope.parentScope = parentScope;


    return let;
}

ArrayConstructionExpr * ast_expr_makeArrayConstructionExpr(){
    ALLOC(array, ArrayConstructionExpr);
    vec_init(&array->args);

    return array;
}

NamedStructConstructionExpr * ast_expr_makeNamedStructConstructionExpr(){
ALLOC(struct_, NamedStructConstructionExpr);
    struct_->type = NULL;
    map_init(&struct_->args);
    vec_init(&struct_->argNames);

    return struct_;
}

UnnamedStructConstructionExpr * ast_expr_makeUnnamedStructConstructionExpr(){
    ALLOC(struct_, UnnamedStructConstructionExpr);
    struct_->type = NULL;
    vec_init(&struct_->args);

    return struct_;
}


LetExprDecl* ast_expr_makeLetExprDecl(){
    ALLOC(decls, LetExprDecl);
    decls->initializerType = LIT_NONE;
    vec_init(&decls->variableNames);
    map_init(&decls->variables);
    decls->initializer = NULL;

    return decls;
}

LiteralExpr* ast_expr_makeLiteralExpr(LiteralType type){
    ALLOC(literal, LiteralExpr);
    literal->type = type;
    literal->value = NULL;

    return literal;
}

Expr* ast_expr_makeExpr(ExpressionType type){
    ALLOC(expr, Expr);
    expr->type = type;
    expr->literalExpr = NULL;
    expr->dataType = NULL;

    return expr;
}

#undef ALLOC