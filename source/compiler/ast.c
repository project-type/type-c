//
// Created by praisethemoon on 28.04.23.
//
#include <stdio.h>
#include "ast.h"
#include "../utils/vec.h"
#include "../utils/map.h"

#define ALLOC(v, t) t* v = malloc(sizeof(t))


ASTProgramNode * ast_makeProgramNode() {
    ASTProgramNode* program = malloc(sizeof(ASTProgramNode));

    vec_init(&program->stmts);
    vec_init(&program->importStatements);
    program->scope = ast_scope_makeScope(NULL);

    return program;
}

PackageID* ast_makePackageID() {
    PackageID* package = malloc(sizeof(PackageID));
    vec_init(&package->ids);

    return package;
}

ImportStmt* ast_makeImportStmt(PackageID* source, PackageID* target, uint8_t hasAlias, char* alias) {
    uint32_t i; char** str;
    PackageID * full_path = ast_makePackageID();
    char* lastName;
    vec_foreach_ptr(&source->ids, str, i) {
        vec_push(&full_path->ids, strdup(*str));
        lastName = *str;
    }
    vec_foreach_ptr(&target->ids, str, i) {
        vec_push(&full_path->ids, strdup(*str));
        lastName = *str;
    }

    ImportStmt * importStmt = malloc(sizeof(ImportStmt));
    importStmt->hasAlias = hasAlias;
    importStmt->alias = alias != NULL? strdup(alias):NULL;
    importStmt->path = full_path;
    importStmt->lookupName = strdup(hasAlias?alias:lastName);

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

VariantType* ast_type_makeVariant(struct ASTScope* parentScope){
    ALLOC(data, VariantType);
    map_init(&data->constructors);
    vec_init(&data->constructorNames);
    data->scope = ast_scope_makeScope(parentScope);

    return data;
}

InterfaceType* ast_type_makeInterface(ASTScope* parentScope) {
    ALLOC(interface, InterfaceType);
    interface->scope = ast_scope_makeScope(parentScope);
    map_init(&interface->methods);
    vec_init(&interface->methodNames);
    vec_init(&interface->extends);

    return interface;
}

ClassType* ast_type_makeClass(ASTScope* parentScope) {
    ALLOC(class, ClassType);
    class->scope = ast_scope_makeScope(parentScope);
    class->scope->withinClass = 1;
    class->scope->classRef = class;

    map_init(&class->methods);
    //map_init(&class->attributes);
    //vec_init(&class->attributeNames);
    vec_init(&class->methodNames);
    vec_init(&class->letList);

    return class;
}

FnType* ast_type_makeFn() {
    ALLOC(fn, FnType);
    map_init(&fn->args);
    vec_init(&fn->argNames);
    fn->returnType = NULL;

    return fn;
}

StructType* ast_type_makeStruct(ASTScope* parentScope) {
    ALLOC(struct_, StructType);
    struct_->scope = ast_scope_makeScope(parentScope);
    map_init(&struct_->attributes);
    vec_init(&struct_->attributeNames);
    vec_init(&struct_->extends);

    return struct_;
}

DataType* ast_type_makeType(Lexeme lexeme) {
    ALLOC(type, DataType);
    type->name = NULL;
    type->hasGenerics = 0;
    type->isGeneric = 0;
    type->isNullable = 0;
    type->kind = DT_UNRESOLVED;
    type->classType = NULL;
    vec_init(&type->genericRefs);
    vec_init(&type->genericNames);
    map_init(&type->generics);
    type->lexeme = lexeme;

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
    header->type = ast_type_makeFn();
    header->isGeneric = 0;

    // init vec and map
    vec_init(&header->genericNames);
    map_init(&header->generics);

    return header;
}

ClassMethod * ast_type_makeClassMethod(){
    ALLOC(method, ClassMethod);
    method->decl = NULL;

    return method;
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
    param->constraint = NULL;

    return param;
}

ProcessType* ast_type_makeProcess(){
    ALLOC(process, ProcessType);
    vec_init(&process->argNames);
    map_init(&process->args);
    process->inputType = NULL;
    process->outputType = NULL;
    process->body = NULL;

    return process;
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
    call->hasGenerics = 0;
    vec_init(&call->generics);

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

IfElseExpr* ast_expr_makeIfElseExpr() {
    ALLOC(ifElse, IfElseExpr);
    ifElse->condition = NULL;
    ifElse->ifExpr = NULL;
    ifElse->elseExpr = NULL;

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
    let->scope = ast_scope_makeScope(parentScope);

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

LambdaExpr* ast_expr_makeLambdaExpr(ASTScope* parentScope){
    ALLOC(lambda, LambdaExpr);
    lambda->scope = ast_scope_makeScope(parentScope);
    lambda->header = NULL;
    lambda->bodyType = FBT_EXPR;
    lambda->expr = NULL;

    return lambda;
}

UnsafeExpr* ast_expr_makeUnsafeExpr(ASTScope* parentScope){
    ALLOC(unsafe, UnsafeExpr);
    unsafe->expr = NULL;
    unsafe->scope = ast_scope_makeScope(parentScope);
    unsafe->scope->isSafe = 0;

    return unsafe;
}

SyncExpr* ast_expr_makeSyncExpr(ASTScope* parentScope){
    ALLOC(sync, SyncExpr);
    sync->expr = NULL;
    sync->scope = ast_scope_makeScope(parentScope);
    sync->scope->withinSync = 1;

    return sync;
}

SpawnExpr* ast_expr_makeSpawnExpr(){
    ALLOC(spawn, SpawnExpr);
    spawn->callback = NULL;
    spawn->expr = NULL;

    return spawn;
}

EmitExpr* ast_expr_makeEmitExpr(){
    ALLOC(emit, EmitExpr);
    emit->msg = NULL;
    emit->process = NULL;

    return emit;
}

ThisExpr* ast_expr_makeThisExpr(){
    ALLOC(this, ThisExpr);
    this->placeHolder = 0;
    return this;
}

/*AwaitExpr* ast_expr_makeAwaitExpr(){
    ALLOC(await, AwaitExpr);
    await->expr = NULL;

    return await;
}*/

Expr* ast_expr_makeExpr(ExpressionType type){
    ALLOC(expr, Expr);
    expr->type = type;
    expr->literalExpr = NULL;
    expr->dataType = NULL;

    return expr;
}

ASTScope * ast_scope_makeScope(ASTScope* parentScope){
    ALLOC(scope, ASTScope);
    scope->isSafe = (parentScope == NULL) ? 1 : parentScope->isSafe;
    scope->withinClass = (parentScope == NULL) ? 0 : parentScope->withinClass;
    scope->withinSync = (parentScope == NULL) ? 0 : parentScope->withinSync;

    scope->parentScope = parentScope;

    map_init(&scope->variables);
    map_init(&scope->functions);
    map_init(&scope->dataTypes);
    vec_init(&scope->externDecls);
    return scope;
}

ExternDecl* ast_externdecl_make(){
    ALLOC(externDecl, ExternDecl);
    externDecl->name = NULL;
    externDecl->linkage = "C";
    vec_init(&externDecl->methodNames);
    map_init(&externDecl->methods);

    return externDecl;
}

BlockStatement* ast_stmt_makeBlockStatement(ASTScope* parentScope){
    ALLOC(block, BlockStatement);
    block->scope = ast_scope_makeScope(parentScope);
    vec_init(&block->stmts);

    return block;
}

VarDeclStatement* ast_stmt_makeVarDeclStatement(ASTScope* parentScope) {
    ALLOC(varDecl, VarDeclStatement);
    varDecl->scope = ast_scope_makeScope(parentScope);
    // init the vector
    vec_init(&varDecl->letList);

    return varDecl;
}

FnDeclStatement* ast_stmt_makeFnDeclStatement(ASTScope* parentScope){
    ALLOC(fnDecl, FnDeclStatement);
    fnDecl->header = ast_makeFnHeader();;
    fnDecl->scope = ast_scope_makeScope(parentScope);
    fnDecl->bodyType = FBT_BLOCK;
    fnDecl->expr = NULL;
    fnDecl->scope->isFn = 1;
    fnDecl->scope->fnHeader = fnDecl->header;

    return fnDecl;
}

IfChainStatement* ast_stmt_makeIfChainStatement(){
    ALLOC(ifChain, IfChainStatement);
    vec_init(&ifChain->conditions);
    vec_init(&ifChain->blocks);
    ifChain->elseBlock = NULL;

    return ifChain;
}

CaseStatement* ast_stmt_makeCaseStatement(){
    ALLOC(caseStmt, CaseStatement);
    caseStmt->condition = NULL;
    caseStmt->block = NULL;

    return caseStmt;
}

MatchStatement* ast_stmt_makeMatchStatement(){
    ALLOC(match, MatchStatement);
    match->expr = NULL;
    vec_init(&match->cases);
    match->elseBlock = NULL;

    return match;
}

WhileStatement* ast_stmt_makeWhileStatement(){
    ALLOC(whileStmt, WhileStatement);
    whileStmt->condition = NULL;
    whileStmt->block = NULL;
    whileStmt->label = NULL;

    return whileStmt;
}

DoWhileStatement* ast_stmt_makeDoWhileStatement(){
    ALLOC(doWhileStmt, DoWhileStatement);
    doWhileStmt->condition = NULL;
    doWhileStmt->block = NULL;
    doWhileStmt->label = NULL;
    return doWhileStmt;
}

ForStatement* ast_stmt_makeForStatement(ASTScope* parentScope){
    ALLOC(forStmt, ForStatement);
    forStmt->scope = ast_scope_makeScope(parentScope);
    forStmt->initializer = NULL;
    forStmt->condition = NULL;
    forStmt->block = NULL;
    // init increments
    vec_init(&forStmt->increments);
    forStmt->label = NULL;

    return forStmt;
}

ForEachStatement* ast_stmt_makeForEachStatement(ASTScope* parentScope){
    ALLOC(forEachStmt, ForEachStatement);
    forEachStmt->scope = ast_scope_makeScope(parentScope);
    forEachStmt->block = NULL;
    forEachStmt->iterable = NULL;
    forEachStmt->variableContentName = NULL;
    forEachStmt->variableIndexName = NULL;

    return forEachStmt;
}

ContinueStatement* ast_stmt_makeContinueStatement(){
    ALLOC(continueStmt, ContinueStatement);
    continueStmt->label = NULL;

    return continueStmt;
}

BreakStatement* ast_stmt_makeBreakStatement() {
    ALLOC(breakStmt, BreakStatement);
    breakStmt->label = NULL;

    return breakStmt;
}

ReturnStatement* ast_stmt_makeReturnStatement(){
    ALLOC(returnStmt, ReturnStatement);
    returnStmt->expr = NULL;

    return returnStmt;
}

UnsafeStatement* ast_stmt_makeUnsafeStatement(ASTScope * parentScope){
    ALLOC(unsafeStmt, UnsafeStatement);
    unsafeStmt->block = NULL;
    unsafeStmt->scope = ast_scope_makeScope(parentScope);
    unsafeStmt->scope->isSafe = 0;

    return unsafeStmt;
}

SyncStatement* ast_stmt_makeSyncStatement(ASTScope * parentScope){
    ALLOC(syncStmt, SyncStatement);
    syncStmt->block = NULL;
    syncStmt->scope = ast_scope_makeScope(parentScope);
    syncStmt->scope->withinSync = 1;

    return syncStmt;
}

ExprStatement* ast_stmt_makeExprStatement(ASTScope * parentScope){
    ALLOC(exprStmt, ExprStatement);
    exprStmt->expr = NULL;

    return exprStmt;
}

Statement* ast_stmt_makeStatement(StatementType type){
    ALLOC(stmt, Statement);
    stmt->type = type;

    return stmt;
}

#undef ALLOC