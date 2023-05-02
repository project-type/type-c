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

char* ast_stringifyImport(ASTProgramNode* node, uint32_t index) {
    // returns string representation of the import statement at the given index
    // we concatenate the path and add "as" alias at the end if it has one
    ImportStmt* importStmt = node->importStatements.data[index];
    char* str = strdup("");
    uint32_t i; char** val;
    vec_foreach_ptr(&importStmt->path->ids, val, i) {
        str = realloc(str, strlen(str) + strlen(*val) + 1);
        strcat(str, *val);
        if(i != importStmt->path->ids.length - 1){
            str = realloc(str, strlen(str) + 1);
            strcat(str, ".");
        }
    }
    if(importStmt->hasAlias){
        str = realloc(str, strlen(str) + strlen(" as ") + strlen(importStmt->alias) + 1);
        strcat(str, " as ");
        strcat(str, importStmt->alias);
    }

    return str;
}

// returns a string representation of the type
char* ast_stringifyType(DataType* type){
    // create base string
    char* str = strdup("");
    if(type->name != NULL){
        str = realloc(str, strlen(str) + strlen(type->name) + 1);
        strcat(str, type->name);
    }
    // if nullable, we add ?
    if(type->isNullable){
        str = realloc(str, strlen(str) + strlen("?") + 1);
        strcat(str, "?");
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
            strcat(str, "[");
            char* arrayType = ast_stringifyType(type->arrayType->arrayOf);
            str = realloc(str, strlen(str) + strlen(arrayType) + 1);
            strcat(str, arrayType);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            char* arraySize = malloc(20);
            sprintf(arraySize, "%llu", type->arrayType->len);
            str = realloc(str, strlen(str) + strlen(arraySize) + 1);
            strcat(str, arraySize);
            str = realloc(str, strlen(str) + strlen(">") + 1);
            strcat(str, "]");

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
            // we check if package is null, if it's not we add it to the string as <p1.p2.p3>
            if(type->refType->pkg != NULL){
                str = realloc(str, strlen(str) + strlen("ref(") + 1);
                strcat(str, "ref(");
                // we loop
                int i; char * val;
                vec_foreach(&type->refType->pkg->ids, val, i) {
                    str = realloc(str, strlen(str) + strlen(val) + 1);
                    strcat(str, val);
                    str = realloc(str, strlen(str) + strlen(".") + 1);
                    strcat(str, ".");
                }
                // we replace the last . with >
                str[strlen(str) - 1] = ')';
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
            // we stringify left and right DataTypes of the union
            char *left = ast_stringifyType(type->unionType->left);
            char *right = ast_stringifyType(type->unionType->right);
            // we concatenate them separate by ","
            str = realloc(str, strlen(str) + strlen(left) + 1);
            strcat(str, left);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            str = realloc(str, strlen(str) + strlen(right) + 1);
            strcat(str, right);
            // close "}"
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");

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
            // we stringify left and right DataTypes of the union
            char *left = ast_stringifyType(type->joinType->left);
            char *right = ast_stringifyType(type->joinType->right);
            // we concatenate them separate by "&"
            str = realloc(str, strlen(str) + strlen(left) + 1);
            strcat(str, left);
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
            str = realloc(str, strlen(str) + strlen(right) + 1);
            strcat(str, right);
            // close "}"
            str = realloc(str, strlen(str) + strlen("}") + 1);
            strcat(str, "}");
            break;

        }
        case DT_STRUCT:{
            // we add all struct fields to string
            str = realloc(str, strlen(str) + strlen("struct") + 1);
            strcat(str, "struct");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->structType->attributeNames, val, i) {
                // we add the name of the attribute
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by :
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                // followed by the type of the attribute
                // first get attribute type from the map
                StructAttribute ** attrType = map_get(&type->structType->attributes, val);
                // then stringify it
                char * attrTypeStr = ast_stringifyType((*attrType)->type);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(attrTypeStr) + 1);
                strcat(str, attrTypeStr);
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");

            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_VARIANT: {
            // we write it as variant{ConstructorName1(arg1: type1, arg2: type2), ConstructorName2(arg1: type1, arg2: type2) , etc)}
            str = realloc(str, strlen(str) + strlen("variant") + 1);
            strcat(str, "variant");
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->variantType->constructorNames, val, i) {
                // we add the name of the constructor
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by (
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                // followed by the arguments
                int j; char * argName;
                // get current constructor
                VariantConstructor ** constructor = map_get(&type->variantType->constructors, val);
                // iterate through the arguments
                vec_foreach(&(*constructor)->argNames, argName, j) {
                    // add the name of the argument
                    str = realloc(str, strlen(str) + strlen(argName) + 1);
                    strcat(str, argName);
                    // followed by :
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // followed by the type of the argument
                    // first get the argument type from the map
                    VariantConstructorArgument ** argType = map_get(&(*constructor)->args, argName);
                    // then stringify it
                    char * argTypeStr = ast_stringifyType((*argType)->type);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                    strcat(str, argTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_INTERFACE: {
            // we write it as interface{FunctionName1(arg1: type1, arg2: type2) -> returnType1, FunctionName2(arg1: type1, arg2: type2) -> returnType2 , etc)}
            str = realloc(str, strlen(str) + strlen("interface") + 1);
            strcat(str, "interface");
            // we add ":" followed "(" <list of extends> ")" if there are any
            if (type->interfaceType->extends.length > 0) {
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                int i; DataType * val;
                vec_foreach(&type->interfaceType->extends, val, i) {
                    // we add the name of the interface
                    char* parentTypeStr = ast_stringifyType(val);
                    str = realloc(str, strlen(str) + strlen(parentTypeStr) + 1);
                    strcat(str, parentTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
            }
            str = realloc(str, strlen(str) + strlen("{") + 1);
            strcat(str, "{");
            int i; char * val;
            vec_foreach(&type->interfaceType->methodNames, val, i) {
                // we add the name of the function
                str = realloc(str, strlen(str) + strlen(val) + 1);
                strcat(str, val);
                // followed by (
                str = realloc(str, strlen(str) + strlen("(") + 1);
                strcat(str, "(");
                // followed by the arguments
                int j; char * argName;
                // get current function
                FnHeader ** function = map_get(&type->interfaceType->methods, val);
                // iterate through the arguments
                vec_foreach(&(*function)->type->argNames, argName, j) {
                    // add the name of the argument
                    str = realloc(str, strlen(str) + strlen(argName) + 1);
                    strcat(str, argName);
                    // followed by :
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // followed by the type of the argument
                    // first get the argument type from the map
                    FnArgument ** argType = map_get(&(*function)->type->args, argName);
                    // then stringify it
                    char * argTypeStr = ast_stringifyType((*argType)->type);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                    strcat(str, argTypeStr);
                    // followed by ,
                    str = realloc(str, strlen(str) + strlen(",") + 1);
                    strcat(str, ",");
                }
                // replace last , with )
                str[strlen(str) - 1] = ')';
                // followed by -> if return type is not NULL
                if((*function)->type->returnType != NULL){
                    str = realloc(str, strlen(str) + strlen("->") + 1);
                    strcat(str, "->");
                    // followed by the return type
                    // first get the return type from the map
                    char * returnType = ast_stringifyType((*function)->type->returnType);
                    // then add it to the string
                    str = realloc(str, strlen(str) + strlen(returnType) + 1);
                    strcat(str, returnType);
                }
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // replace last , with }
            str[strlen(str) - 1] = '}';
            break;
        }
        case DT_FN: {
            // return fn(arg1: type1, arg2: type2) -> returnType
            str = realloc(str, strlen(str) + strlen("fn") + 1);
            strcat(str, "fn");
            // followed by (
            str = realloc(str, strlen(str) + strlen("(") + 1);
            strcat(str, "(");
            // followed by the arguments
            int i; char * argName;
            vec_foreach(&type->fnType->argNames, argName, i) {
                // add the name of the argument
                str = realloc(str, strlen(str) + strlen(argName) + 1);
                strcat(str, argName);
                // followed by :
                str = realloc(str, strlen(str) + strlen(":") + 1);
                strcat(str, ":");
                // followed by the type of the argument
                // first get the argument type from the map
                FnArgument ** argType = map_get(&type->fnType->args, argName);
                // then stringify it
                char * argTypeStr = ast_stringifyType((*argType)->type);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(argTypeStr) + 1);
                strcat(str, argTypeStr);
                // followed by ,
                str = realloc(str, strlen(str) + strlen(",") + 1);
                strcat(str, ",");
            }
            // if we had args, we replace last , with )
            if(type->fnType->argNames.length > 0){
                str[strlen(str) - 1] = ')';
            } else {
                // if we didn't have args, we just add )
                str = realloc(str, strlen(str) + strlen(")") + 1);
                strcat(str, ")");
            }

            // add return type if it exists
            if(type->fnType->returnType != NULL){
                // followed by -> if return type is not NULL
                str = realloc(str, strlen(str) + strlen("->") + 1);
                strcat(str, "->");
                // followed by the return type
                // first get the return type from the map
                char * returnType = ast_stringifyType(type->fnType->returnType);
                // then add it to the string
                str = realloc(str, strlen(str) + strlen(returnType) + 1);
                strcat(str, returnType);
            }
            break;
        }
        case DT_PTR:{
            // return ptr<type>
            str = realloc(str, strlen(str) + strlen("ptr") + 1);
            strcat(str, "ptr");
            // followed by <
            str = realloc(str, strlen(str) + strlen("<") + 1);
            strcat(str, "<");
            // followed by the type
            char * ptrType = ast_stringifyType(type->ptrType->target);
            str = realloc(str, strlen(str) + strlen(ptrType) + 1);
            strcat(str, ptrType);
            // followed by >
            str = realloc(str, strlen(str) + strlen(">") + 1);
            strcat(str, ">");
            break;
        }
        default:
            break;
    }

    // we check if it has generics
    if(type->hasGeneric){
        // we add the generics to the string
        int i; GenericParam * val;
        // first we add a "<" to the string
        str = realloc(str, strlen(str) + 1);
        strcat(str, "<");
        // then we add the generics
        vec_foreach(&type->genericParams, val, i) {
            // we check if the genericParam is generic or not
            if(val->isGeneric){
                // if it is, we add the name of the genericParam
                str = realloc(str, strlen(str) + strlen(val->name) + 1);
                strcat(str, val->name);
                if(val->constraint != NULL) {
                    // add ":"
                    str = realloc(str, strlen(str) + strlen(":") + 1);
                    strcat(str, ":");
                    // if it is not, we add the type of the genericParam
                    char * genericType = ast_stringifyType(val->constraint);
                    str = realloc(str, strlen(str) + strlen(genericType) + 1);
                    strcat(str, genericType);
                }
            } else {
                // if it's not generic then it is a concrete type, we serialize its type
                char * genericType = ast_stringifyType(val->type);
                str = realloc(str, strlen(str) + strlen(genericType) + 1);
                strcat(str, genericType);
            }
            // followed by ,
            str = realloc(str, strlen(str) + strlen(",") + 1);
            strcat(str, ",");
        }
        // replace last , with >
        str[strlen(str) - 1] = '>';

    }
    return str;
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

#undef ALLOC