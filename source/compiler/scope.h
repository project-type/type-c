//
// Created by praisethemoon on 09.05.23.
//

#ifndef TYPE_C_SCOPE_H
#define TYPE_C_SCOPE_H

#include "ast.h"
#include "parser.h"

typedef enum ASTScopeResultType {
    SCOPE_UNDEFINED, // This is an error
    SCOPE_VARIABLE,
    SCOPE_METHOD,
    SCOPE_ATTRIBUTE,
    SCOPE_ARGUMENT,
    SCOPE_FUNCTION,
    SCOPE_TYPE,
    SCOPE_FFI,
    SCOPE_MODULE,
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

typedef enum ScopeLookResult {
    SLRT_TOKEN_NOT_DEFINED=0,
    SLRT_SUCCESS=1,
}ScopeLookResult;

uint8_t scope_isSafe(ASTScope* scope);

ScopeRegResult scope_program_addImport(ASTProgramNode* program, ImportStmt * import);

ScopeRegResult scope_ffi_addMethod(ExternDecl* ffi, FnHeader* method);

ScopeRegResult scope_fnheader_addGeneric(FnHeader* fn, GenericParam * genericParam);
ScopeRegResult scope_fnheader_addArg(FnHeader* fn, FnArgument* arg);
ScopeRegResult scope_dtype_addGeneric(DataType* dtype, GenericParam * genericParam);

uint8_t scope_canExtend(DataType* parent, DataTypeKind childKind);

char* scope_extends_addParent(ASTScope * scope, vec_dtype_t* extends, DataType* parent);
char* scope_interface_addMethod(DataType * interface, FnHeader* method);
char* scope_class_addMethod(DataType * class, ClassMethod* fnDecl);
char* scope_class_addAttribute(DataType * class, LetExprDecl* decl);
char* scope_struct_addAttribute(DataType * struct_, StructAttribute* attr);
ScopeRegResult scope_process_AddArg(ProcessType * process, FnArgument * arg);
ScopeRegResult scope_process_hasReceive(ProcessType * process);
ScopeRegResult scope_process_receiveMatches(ProcessType * process);

ScopeRegResult scope_variantConstructor_addArg(VariantConstructor* constructor, VariantConstructorArgument* arg);
ScopeRegResult scope_variant_addConstructor(VariantType * variant, VariantConstructor * constructor);

ScopeRegResult scope_fntype_addArg(FnType* fn, FnArgument* arg);


ScopeRegResult scope_registerFFI(ASTScope* scope, ExternDecl* ffi);
ScopeRegResult scope_registerVariable(ASTScope* scope, FnArgument* variable);
ScopeRegResult scope_registerFunction(ASTScope* scope, FnDeclStatement* fnDecl);

ScopeRegResult scope_registerType(ASTScope* scope, DataType* dataType);

//void scope_registerModule(ASTScope* scope, ASTModuleDecl* module);

// data lookup
ASTScopeResultType scope_lookupSymbol(ASTScope* scope, char* name);
DataType* scope_lookupVariable(ASTScope* scope, char* name);
DataType* scope_lookupFunction(ASTScope* scope, char* name);
DataType* scope_getClassRef(ASTScope* scope);
/**
 * Validating data
 */
ASTScopeResult* scope_result_init(ASTScopeResultType type, void* data);
ASTScopeResult* resolveElement(char* e, ASTScope* scope, uint8_t recursive);

#endif //TYPE_C_SCOPE_H
