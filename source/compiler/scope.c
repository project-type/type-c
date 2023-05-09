//
// Created by praisethemoon on 09.05.23.
//

#include "scope.h"
#include "ast.h"

ScopeRegResult scope_program_addImport(ASTProgramNode* program, ImportStmt * import){
    // make sure lookup name doesn't exist already
    ImportStmt* imp; uint32_t i;
    vec_foreach(&program->importStatements, imp, i){
        if(strcmp(imp->lookupName, import->lookupName) == 0){
            return SRRT_TOKEN_ALREADY_REGISTERED;
        }
    }

    vec_push(&program->importStatements, import);
    return SRRT_SUCCESS;
}

ScopeRegResult scope_ffi_addMethod(ExternDecl* ffi, FnHeader* method){
    // make sure function name doesn't exist already
    if(map_get(&ffi->methods, method->name) == NULL){
        vec_push(&ffi->methodNames, method->name);
        map_set(&ffi->methods, method->name, method);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_fnheader_addGeneric(FnHeader* fn, GenericParam * genericParam) {
    // make sure generic name doesn't exist already
    if(map_get(&fn->generics, genericParam->name) == NULL){
        vec_push(&fn->genericNames, genericParam->name);
        map_set(&fn->generics, genericParam->name, genericParam);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_fnheader_addArg(FnHeader* fn, FnArgument* arg){
    // make sure argument name doesn't exist already
    if(map_get(&fn->type->args, arg->name) == NULL){
        vec_push(&fn->type->argNames, arg->name);
        map_set(&fn->type->args, arg->name, arg);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_dtype_addGeneric(DataType* dtype, GenericParam * genericParam){
    // make sure generic name doesn't exist already
    if(map_get(&dtype->generics, genericParam->name) == NULL){
        vec_push(&dtype->genericNames, genericParam->name);
        map_set(&dtype->generics, genericParam->name, genericParam);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}


ScopeRegResult scope_registerType(ASTScope* scope, DataType* dataType){
    // make sure no shadowing allowed
    if(resolveElement(dataType->name, scope, 0) == NULL) {
        map_set(&scope->dataTypes, dataType->name, dataType);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_registerFFI(ASTScope* scope, ExternDecl* ffi){
    // make sure no shadowing allowed
    if(resolveElement(ffi->name, scope, 0) == NULL) {
        map_set(&scope->externDecls, ffi->name, ffi);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ASTScopeResult* scope_result_init(ASTScopeResultType type, void* data){
    ASTScopeResult* result = malloc(sizeof(ASTScopeResult));
    result->type = type;
    switch (type) {
        case SCOPE_VARIABLE:
            result->variable = data;
            break;
        case SCOPE_FUNCTION:
            result->function = data;
            break;
        case SCOPE_TYPE:
            result->dataType = data;
            break;
        case SCOPE_FFI:
            result->ffi = data;
            break;
        case SCOPE_MODULE:
            //result->module = data;
            break;
    }
    return result;
}

ASTScopeResult* resolveElement(char* e, ASTScope* scope, uint8_t recursive) {
    // check if scope is NULL
    if (scope == NULL) {
        return NULL;
    }
    // check if the element is a variable
    FnArgument ** variable = map_get(&scope->variables, e);
    if (variable != NULL) {
        return scope_result_init(SCOPE_VARIABLE, *variable);
    }

    // check if the element is a function
    FnHeader ** function = map_get(&scope->functions, e);
    if (function != NULL) {
        return scope_result_init(SCOPE_FUNCTION, *function);
    }

    // check if the element is a type
    DataType ** dataType = map_get(&scope->dataTypes, e);
    if (dataType != NULL) {
        return scope_result_init(SCOPE_TYPE, *dataType);
    }

    // check if the element is a ffi
    ExternDecl ** ffi = map_get(&scope->externDecls, e);
    if (ffi != NULL) {
        return scope_result_init(SCOPE_FFI, *ffi);
    }

    if((scope->parentScope != NULL) && recursive){
        return resolveElement(e, scope->parentScope, recursive);
    }

    return NULL;
}