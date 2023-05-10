//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_SCOPE_H
#define TYPE_C_SCOPE_H

#include "ast.h"
#include "parser.h"

typedef enum ASTScopeResultType {
    SCOPE_VARIABLE,
    SCOPE_FUNCTION,
    SCOPE_TYPE,
    SCOPE_FFI,
    SCOPE_MODULE
}ASTScopeResultType;

typedef struct ASTScopeResult {
    ASTScopeResultType type;
    union {
        FnArgument * variable;
        FnHeader * function;
        DataType * dataType;
        ExternDecl * ffi;
        //ASTModuleDecl* module;
    };
}ASTScopeResult;

/**
 * Creating data
 */

typedef enum ScopeRegResult {
    SRRT_TOKEN_ALREADY_REGISTERED=0,
    SRRT_SUCCESS=1,
}ScopeRegResult;


ScopeRegResult scope_program_addImport(ASTProgramNode* program, ImportStmt * import);

ScopeRegResult scope_ffi_addMethod(ExternDecl* ffi, FnHeader* method);

ScopeRegResult scope_fnheader_addGeneric(FnHeader* fn, GenericParam * genericParam);
ScopeRegResult scope_fnheader_addArg(FnHeader* fn, FnArgument* arg);
ScopeRegResult scope_dtype_addGeneric(DataType* dtype, GenericParam * genericParam);

uint8_t scope_canExtend(DataType* parent, DataTypeKind childKind);
char* scope_extends_addParent(ASTScope * scope, vec_dtype_t* extends, DataType* parent);

ScopeRegResult scope_interface_addMethod(InterfaceType * interface, FnHeader* method);

ScopeRegResult scope_variantConstructor_addArg(VariantConstructor* constructor, VariantConstructorArgument* arg);
ScopeRegResult scope_variant_addConstructor(VariantType * variant, VariantConstructor * constructor);

ScopeRegResult scope_registerFFI(ASTScope* scope, ExternDecl* ffi);
ScopeRegResult scope_registerVariable(ASTScope* scope, FnArgument* variable);
ScopeRegResult scope_registerFunction(ASTScope* scope, FnHeader* function);
ScopeRegResult scope_registerType(ASTScope* scope, DataType* dataType);

//void scope_registerModule(ASTScope* scope, ASTModuleDecl* module);

/**
 * Validating data
 */
ASTScopeResult* scope_result_init(ASTScopeResultType type, void* data);
ASTScopeResult* resolveElement(char* e, ASTScope* scope, uint8_t recursive);

#endif //TYPE_C_SCOPE_H
