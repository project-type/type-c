//
// Created by praisethemoon on 09.05.23.
//

#include "scope.h"
#include "ast.h"
#include "type_checker.h"
#include "error.h"

uint8_t scope_isSafe(ASTScope* scope){
    return scope->isSafe;
}

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

        // add generic to scope to
        vec_push(&dtype->scope->genericNames, genericParam->name);
        map_set(&dtype->scope->generics, genericParam->name, genericParam);

        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

uint8_t scope_canExtend(DataType* parent, DataTypeKind childKind){
    return tc_gettype_base(parent) == childKind;
}

char* scope_extends_addParent(ASTScope * scope, vec_dtype_t* extends, DataType* parent){
    map_int_t map;
    map_init(&map);
    char* res1 = tc_accumulate_type_methods_attribute(parent, &map);
    if(res1 != NULL){
        return res1;
    }

    if(parent->kind == DT_TYPE_JOIN){
        char* duplicate = tc_check_canJoin(parent->joinType->left, parent->joinType->right);
        if(duplicate != NULL){
            return duplicate;
        }
    }

    uint32_t i = 0;
    DataType * type = NULL;
    vec_foreach(extends, type, i){
        char* res = tc_accumulate_type_methods_attribute(type, &map);
        if(res != NULL){
            return res;
        }
    }

    vec_push(extends, parent);
    return NULL;
}

ScopeRegResult scope_registerType(ASTScope* scope, DataType* dataType){
    // make sure no shadowing allowed
    if(resolveElement(dataType->name, scope, 0) == NULL) {
        map_set(&scope->dataTypes, dataType->name, dataType);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

char* scope_interface_addMethod(DataType * interface, FnHeader* method){
    ASSERT(interface->kind == DT_INTERFACE, "Input is not an interface");

    map_set(&interface->interfaceType->methods, method->name, method);
    vec_push(&interface->interfaceType->methodNames, method->name);

    map_int_t map;
    map_init(&map);
    // check current interface
    char* res = tc_accumulate_type_methods_attribute(interface, &map);
    if(res != NULL){
        return res;
    }

    return NULL;
}

char* scope_class_addMethod(DataType * class, ClassMethod* fnDecl){
    ASSERT(class->kind == DT_CLASS, "Input is not an interface");

    map_int_t map;
    map_init(&map);

    vec_push(&class->classType->methodNames, fnDecl->decl->header->name);
    map_set(&class->classType->methods, fnDecl->decl->header->name, fnDecl);

    // check current interface
    char* dup = tc_accumulate_type_methods_attribute(class, &map);
    if(dup != NULL) {
        return dup;
    }

    return NULL;
}

char* scope_class_addAttribute(DataType * class, LetExprDecl* decl){
    ASSERT(class->kind == DT_CLASS, "Input is not an interface");
    vec_push(&class->classType->letList, decl);

    map_int_t map;
    map_init(&map);
    // check current interface
    char* dup = tc_accumulate_type_methods_attribute(class, &map);
    if(dup != NULL) {
        return dup;
    }

    return NULL;
}

char* scope_struct_addAttribute(DataType * struct_, StructAttribute* attr){
    ASSERT(struct_->kind == DT_STRUCT, "Input is not a struct");

    vec_push(&struct_->structType->attributeNames, attr->name);
    map_set(&struct_->structType->attributes, attr->name, attr);

    map_int_t map;
    map_init(&map);
    // check current interface
    char* dup = tc_accumulate_type_methods_attribute(struct_, &map);
    if(dup != NULL) {
        return dup;
    }

    return NULL;
}

ScopeRegResult scope_variantConstructor_addArg(VariantConstructor* constructor, VariantConstructorArgument* arg){
    // make sure argument name doesn't exist already
    if(map_get(&constructor->args, arg->name) == NULL){
        vec_push(&constructor->argNames, arg->name);
        map_set(&constructor->args, arg->name, arg);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_variant_addConstructor(VariantType * variant, VariantConstructor * constructor){
    // make sure constructor name doesn't exist already
    if(map_get(&variant->constructors, constructor->name) == NULL){
        vec_push(&variant->constructorNames, constructor->name);
        map_set(&variant->constructors, constructor->name, constructor);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_process_AddArg(ProcessType * process, FnArgument * arg){
    // make sure argument name doesn't exist already
    if(map_get(&process->args, arg->name) == NULL){
        vec_push(&process->argNames, arg->name);
        map_set(&process->args, arg->name, arg);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_process_hasReceive(ProcessType * process){
    // scope must have one statement only, and it must be a function called receive
    if(process->body->blockStmt->stmts.length == 1){
        Statement * stmt = vec_last(&process->body->blockStmt->stmts);
        if(stmt->type == ST_FN_DECL){
            FnDeclStatement* fn = stmt->fnDecl;
            if(strcmp(fn->header->name, "receive") == 0){
                return SRRT_SUCCESS;
            }

        }
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_fntype_addArg(FnType* fn, FnArgument* arg){
    // make sure argument name doesn't exist already
    if(map_get(&fn->args, arg->name) == NULL){
        vec_push(&fn->argNames, arg->name);
        map_set(&fn->args, arg->name, arg);
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

ScopeRegResult scope_registerVariable(ASTScope* scope, FnArgument* variable){
    // make sure no shadowing allowed
    if(resolveElement(variable->name, scope, 0) == NULL) {
        map_set(&scope->variables, variable->name, variable);
        return SRRT_SUCCESS;
    }

    return SRRT_TOKEN_ALREADY_REGISTERED;
}

ScopeRegResult scope_registerFunction(ASTScope* scope, FnDeclStatement* fnDecl){
    // make sure no shadowing allowed
    if(resolveElement(fnDecl->header->name, scope, 0) == NULL) {
        map_set(&scope->functions, fnDecl->header->name, fnDecl);
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

/**
* Lookup
*/


ASTScopeResultType scope_lookupSymbol(ASTScope* scope, char* name){
    // check if scope is NULL
    if(scope == NULL){
        return SCOPE_UNDEFINED;
    }

    // check if variable is defined in current scope
    if(map_get(&scope->variables, name) != NULL){
        return SCOPE_VARIABLE;
    }

    // check if function is defined in current scope
    if(map_get(&scope->functions, name) != NULL){
        return SCOPE_FUNCTION;
    }

    // check if type is defined in current scope
    if(map_get(&scope->dataTypes, name) != NULL){
        return SCOPE_TYPE;
    }

    // check if extern is defined in current scope
    if(map_get(&scope->externDecls, name) != NULL){
        return SCOPE_FFI;
    }


    // if scope is function we check its args
    if(scope->isFn){
        // fetch args
        FnArgument ** arg = map_get(&scope->fnHeader->type->args, name);
        if(arg != NULL){
            return SCOPE_ARGUMENT;
        }
    }

    if((scope->withinClass) && (scope->classRef != NULL) && (scope == scope->classRef->classType->scope)){
        // check methods
        ClassMethod ** method = map_get(&scope->classRef->classType->methods, name);
        if(method != NULL){
            return SCOPE_METHOD;
        }
        // iterate through letList
        for(uint32_t i = 0; i < scope->classRef->classType->letList.length; i++){
            struct LetExprDecl * let = scope->classRef->classType->letList.data[i];
            // look up let->variableNames
            for(uint32_t j = 0; j < let->variableNames.length; j++){
                char * varName = let->variableNames.data[j];
                if(strcmp(varName, name) == 0){
                    return SCOPE_ATTRIBUTE;
                }
            }
        }
    }
    // check if variable is defined in parent scope
    if(scope->parentScope != NULL) {
        return scope_lookupSymbol(scope->parentScope, name);
    }

    return SCOPE_UNDEFINED;
}

FnHeader* scope_getFnRef(ASTScope* scope){
    // returns the function of the current scope
    if(scope == NULL){
        return NULL;
    }

    if(scope->isFn){
        return scope->fnHeader;
    }

    return scope_getFnRef(scope->parentScope);
}

DataType* scope_lookupVariable(ASTScope* scope, char* name){
    // searches for variables, argument or attribute and returns its type
    if(scope == NULL){
        return NULL;
    }

    // check if variable is defined in current scope
    FnArgument ** variable = map_get(&scope->variables, name);
    if(variable != NULL){
        return (*variable)->type;
    }

    // if scope is function we check its args
    if(scope->isFn){
        // fetch args
        FnArgument ** arg = map_get(&scope->fnHeader->type->args, name);
        if(arg != NULL){
            return (*arg)->type;
        }
    }

    // check class attributes
    if((scope->withinClass) && (scope->classRef != NULL) && (scope == scope->classRef->classType->scope)){
        // iterate through letList
        for(uint32_t i = 0; i < scope->classRef->classType->letList.length; i++){
            struct LetExprDecl * let = scope->classRef->classType->letList.data[i];
            // look up let->variableNames
            for(uint32_t j = 0; j < let->variableNames.length; j++){
                char * varName = let->variableNames.data[j];
                if(strcmp(varName, name) == 0){
                    FnArgument ** arg = map_get(&let->variables, name);
                    return (*arg)->type;
                }
            }
        }
    }

    // check if variable is defined in parent scope
    if(scope->parentScope != NULL) {
        return scope_lookupVariable(scope->parentScope, name);
    }

    return NULL;
}

DataType* scope_lookupFunction(ASTScope* scope, char* name){
    // searches for functions and returns its type
    if(scope == NULL){
        return NULL;
    }

    // check if function is defined in current scope
    FnDeclStatement ** function = map_get(&scope->functions, name);
    if(function != NULL){
        return (*function)->dataType;
    }

    // if class check methods
    if((scope->withinClass) && (scope->classRef != NULL) && (scope == scope->classRef->classType->scope)){
        // check methods
        ClassMethod ** method = map_get(&scope->classRef->classType->methods, name);
        if(method != NULL){
            return (*method)->decl->dataType;
        }
    }

    // check if function is defined in parent scope
    if(scope->parentScope != NULL) {
        return scope_lookupFunction(scope->parentScope, name);
    }

    return NULL;
}

DataType* scope_getClassRef(ASTScope* scope){
    ASSERT(scope->withinClass, "Scope is not within a class");

    if(scope->classRef != NULL){
        return scope->classRef;
    }

    if((scope->parentScope != NULL) && (scope->parentScope->withinClass)){
        return scope_getClassRef(scope->parentScope);
    }

    return NULL;
}