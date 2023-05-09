//
// Created by praisethemoon on 05.05.23.
//

#include "ast_json.h"
#include "ast.h"
#include "../utils/parson.h"
#include "../utils/vec.h"
#include "../utils/map.h"
#include "../utils/sds.h"
#include "error.h"

char* ast_stringifyUnaryExprType(UnaryExprType type){
    switch (type) {
        case UET_NOT:
            return "!";
        case UET_BIT_NOT:
            return "~";
        case UET_NEG:
            return "-";
        case UET_PRE_INC:
            return "++";
        case UET_PRE_DEC:
            return "--";
        case UET_POST_INC:
            return "++";
        case UET_POST_DEC:
            return "--";
        case UET_DEREF:
            return "*";
        case UET_ADDRESS_OF:
            return "&";
        case UET_DENULL:
            return "!!";
        default:
            ASSERT(0, "Unknown unary expression type %d", type);
            return "unknown";
    }
}

char* ast_stringifyBinaryExprType(BinaryExprType type){
    switch (type) {
        case BET_ADD:
            return "+";
        case BET_SUB:
            return "-";
        case BET_MUL:
            return "*";
        case BET_DIV:
            return "/";
        case BET_MOD:
            return "%";
        case BET_AND:
            return "&&";
        case BET_OR:
            return "||";
        case BET_BIT_XOR:
            return "^";
        case BET_BIT_AND:
            return "&";
        case BET_BIT_OR:
            return "|";
        case BET_LSHIFT:
            return "<<";
        case BET_RSHIFT:
            return ">>";
        case BET_EQ:
            return "==";
        case BET_NEQ:
            return "!=";
        case BET_LT:
            return "<";
        case BET_LTE:
            return "<=";
        case BET_GT:
            return ">";
        case BET_GTE:
            return ">=";
        case BET_ASSIGN:
            return "=";
        case BET_ADD_ASSIGN:
            return "+=";
        case BET_SUB_ASSIGN:
            return "-=";
        case BET_MUL_ASSIGN:
            return "*=";
        case BET_DIV_ASSIGN:
            return "/=";
        default:
            ASSERT(0, "Unknown binary expression type %d", type);
            return "unknown";
    }
}

char* ast_json_serializeImports(ASTProgramNode* node){
    // we need to serialize the imports
    /*
     * [{path: "path/to/file", alias: "alias"}, ...}]
     */
    JSON_Value* root_value = json_value_init_array();
    JSON_Array* root_array = json_value_get_array(root_value);
    for (uint32_t i = 0; i < node->importStatements.length; ++i) {
        ImportStmt* importNode = node->importStatements.data[i];
        JSON_Value* import_value = json_value_init_object();
        JSON_Object* import_object = json_value_get_object(import_value);
        // convert vec to str
        sds path = sdsempty();
        uint32_t j; char* name;
        for (j = 0; j < importNode->path->ids.length; ++j) {
            name = importNode->path->ids.data[j];
            path = sdscat(path, name);
            if (j != importNode->path->ids.length - 1){
                path = sdscat(path, ".");
            }
        }

        json_object_set_string(import_object, "path", path);
        json_object_set_string(import_object, "alias", importNode->alias);
        json_array_append_value(root_array, import_value);
    }

    char* json_string = json_serialize_to_string(root_value);
    return json_string;
}




JSON_Value* ast_json_serializeDataTypeRecursive(DataType* type) {
    // we need to serialize the datatype,
    // {name: "name", type: "type", isPointer: true/false, etc
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    // set the name
    if(type->name!=NULL)
        json_object_set_string(root_object, "name", type->name);
    else
        json_object_set_null(root_object, "name");
    // set isNullable
    json_object_set_boolean(root_object, "isNullable", type->isNullable);
    // set hasGeneric
    json_object_set_boolean(root_object, "hasGeneric", type->hasGeneric);
    // check if type has generic
    if (type->hasGeneric){
        // create new array which holds generic
        JSON_Value* generic_value = json_value_init_array();
        JSON_Array* generic_array = json_value_get_array(generic_value);
        // iterate over the generic
        int uint32_t; GenericParam * val;
        vec_foreach(&type->genericParams, val, uint32_t){
            // create generic object
            JSON_Value* generic_object_value = json_value_init_object();
            JSON_Object* generic_object = json_value_get_object(generic_object_value);
            // set isGeneric
            json_object_set_boolean(generic_object, "isGeneric", val->isGeneric);
            // check isGeneric
            if (val->isGeneric){
                // set the name
                json_object_set_string(generic_object, "name", val->name);
                // set the constraint if its not null
                if (val->constraint != NULL)
                    json_object_set_value(generic_object, "constraint", ast_json_serializeDataTypeRecursive(val->constraint));
                else
                    // set constraint to null
                    json_object_set_null(generic_object, "constraint");
            } else {
                // set the type
                json_object_set_value(generic_object, "type", ast_json_serializeDataTypeRecursive(val->type));
            }
            // append to array
            json_array_append_value(generic_array, generic_object_value);
        }
        // set the generic array
        json_object_set_value(root_object, "genericParams", generic_value);

    }

    // now depending on the kind, we might store different things
    switch(type->kind){
        case DT_I8:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = i8
            json_object_set_string(root_object, "primitiveType", "i8");
            break;
        case DT_I16:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = i16
            json_object_set_string(root_object, "primitiveType", "i16");
            break;
        case DT_I32:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = i32
            json_object_set_string(root_object, "primitiveType", "i32");
            break;
        case DT_I64:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = i64
            json_object_set_string(root_object, "primitiveType", "i64");
            break;
        case DT_U8:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = u8
            json_object_set_string(root_object, "primitiveType", "u8");
            break;
        case DT_U16:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = u16
            json_object_set_string(root_object, "primitiveType", "u16");
            break;
        case DT_U32:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = u32
            json_object_set_string(root_object, "primitiveType", "u32");
            break;
        case DT_U64:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = u64
            json_object_set_string(root_object, "primitiveType", "u64");
            break;
        case DT_F32:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = f32
            json_object_set_string(root_object, "primitiveType", "f32");
            break;
        case DT_F64:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = f64
            json_object_set_string(root_object, "primitiveType", "f64");
            break;
        case DT_BOOL:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = bool
            json_object_set_string(root_object, "primitiveType", "bool");
            break;
        case DT_VOID: {
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = void
            json_object_set_string(root_object, "primitiveType", "void");
            break;
        }
        case DT_VEC: {
            /*
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = vec
            json_object_set_string(root_object, "primitiveType", "vec");
            break;
             */
            break;
        }
        case DT_STRING:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = string
            json_object_set_string(root_object, "primitiveType", "string");
            break;
        case DT_CHAR:
            // add category=primitive
            json_object_set_string(root_object, "category", "primitive");
            // add primitive type = char
            json_object_set_string(root_object, "primitiveType", "char");
            break;
        case DT_CLASS: {
            // category=class
            json_object_set_string(root_object, "category", "class");
            // first we check what the class extends
            JSON_Value* extends_value = json_value_init_array();
            JSON_Array* extends_array = json_value_get_array(extends_value);
            // iterate over the extends
            uint32_t i ; DataType * val;
            vec_foreach(&type->classType->extends, val, i){
                // add the type
                json_array_append_value(extends_array, ast_json_serializeDataTypeRecursive(val));
            }
            // add the extends
            json_object_set_value(root_object, "extends", extends_value);
            // now we add the methods
            JSON_Value* methods_value = json_value_init_array();
            JSON_Array* methods_array = json_value_get_array(methods_value);
            char* methodName;
            // iterate over the methods

            vec_foreach(&type->classType->methodNames, methodName, i){
                ClassMethod ** function = map_get(&type->classType->methods, methodName);
                FnDeclStatement* fnDecl = (*function)->decl;
                // create the object to hold method decl
                JSON_Value* method_value = json_value_init_object();
                JSON_Object* method_object = json_value_get_object(method_value);

                json_object_set_string(method_object, "bodyType", fnDecl->bodyType==FBT_EXPR?"expr":"block");
                // add the name
                json_object_set_string(method_object, "name", fnDecl->header->name);
                // add the return type if exists, else set value to null
                if(fnDecl->header->type->returnType != NULL)
                    json_object_set_value(method_object, "returnType", ast_json_serializeDataTypeRecursive(fnDecl->header->type->returnType));
                else
                    json_object_set_null(method_object, "returnType");
                // add function isGeneric
                json_object_set_boolean(method_object, "hasGeneric", fnDecl->hasGeneric);
                // add generic params in an array
                JSON_Value * genericParams_value = json_value_init_array();
                JSON_Array * genericParams_array = json_value_get_array(genericParams_value);
                // iterate over the generic params
                uint32_t j = 0; GenericParam * genericParam;
                vec_foreach(&fnDecl->genericParams, genericParam, j) {
                        // create a generic param object
                        JSON_Value * genericParam_value = json_value_init_object();
                        JSON_Object * genericParam_object = json_value_get_object(genericParam_value);
                        // add the name
                        json_object_set_string(genericParam_object, "name", genericParam->name);
                        // add the requirements or null if it's null
                        if (genericParam->constraint != NULL)
                            json_object_set_value(genericParam_object, "supertype", ast_json_serializeDataTypeRecursive(genericParam->constraint));
                        else
                            json_object_set_null(genericParam_object, "requirements");


                        // add the generic param to the generic params array
                        json_array_append_value(genericParams_array, genericParam_value);
                    }
                // add the generic params array to the root object
                json_object_set_value(method_object, "genericParams", genericParams_value);


                // add the args
                // create an array of args
                JSON_Value * args_value = json_value_init_array();
                JSON_Array * args_array = json_value_get_array(args_value);
                // iterate over the args
                char * argName;
                vec_foreach(&fnDecl->header->type->argNames, argName, i) {
                        FnArgument** arg = map_get(&fnDecl->header->type->args, argName);
                        // create an arg object
                        JSON_Value * arg_value = json_value_init_object();
                        JSON_Object * arg_object = json_value_get_object(arg_value);
                        // add the name
                        json_object_set_string(arg_object, "name", argName);
                        // add the type
                        json_object_set_value(arg_object, "type", ast_json_serializeDataTypeRecursive((*arg)->type));
                        // add the arg to the args array
                        json_array_append_value(args_array, arg_value);
                        // add isMutable
                        json_object_set_boolean(arg_object, "isMutable", (*arg)->isMutable);
                        // add to array
                        json_array_append_value(args_array, arg_value);
                    }
                // add the args array to the root object
                json_object_set_value(method_object, "args", args_value);

                // if body type is expression we set expr
                if (fnDecl->bodyType == FBT_EXPR)
                    json_object_set_value(method_object, "expr", ast_json_serializeExprRecursive(fnDecl->expr));
                else
                    json_object_set_value(method_object, "block", ast_json_serializeStatementRecursive(fnDecl->block));

                json_array_append_value(methods_array, method_value);
            }
            // add the methods
            json_object_set_value(root_object, "methods", methods_value);


            break;
        }
        case DT_INTERFACE: {
            // category=interface
            json_object_set_string(root_object, "category", "interface");
            // first we check what the interface extends
            JSON_Value* extends_value = json_value_init_array();
            JSON_Array* extends_array = json_value_get_array(extends_value);
            // iterate over the extends
            uint32_t i ; DataType * val;
            vec_foreach(&type->interfaceType->extends, val, i){
                // add the type
                json_array_append_value(extends_array, ast_json_serializeDataTypeRecursive(val));
            }
            // add the extends
            json_object_set_value(root_object, "extends", extends_value);
            // now we add the methods
            JSON_Value* methods_value = json_value_init_array();
            JSON_Array* methods_array = json_value_get_array(methods_value);
            // iterate over the methods
            char * methodName;
            vec_foreach(&type->interfaceType->methodNames, methodName, i){
                // we want to create an object {"name": <name>, "args": [<args>], "returnType": <returnType>}
                JSON_Value* method_value = json_value_init_object();
                JSON_Object* method_object = json_value_get_object(method_value);
                // add the name
                json_object_set_string(method_object, "name", methodName);
                // add the args
                JSON_Value* args_value = json_value_init_array();
                JSON_Array* args_array = json_value_get_array(args_value);
                // iterate over the args
                FnHeader ** function = map_get(&type->interfaceType->methods, methodName);
                uint32_t j; char* argName;
                vec_foreach(&(*function)->type->argNames, argName, j) {
                    FnArgument ** argType = map_get(&(*function)->type->args, argName);
                    // we want to create an object {"name": <name>, "type": <type>}
                    JSON_Value* arg_value = json_value_init_object();
                    JSON_Object* arg_object = json_value_get_object(arg_value);
                    // add the name
                    json_object_set_string(arg_object, "name", argName);
                    // add the type
                    json_object_set_value(arg_object, "type", ast_json_serializeDataTypeRecursive((*argType)->type));
                    // add the arg
                    json_array_append_value(args_array, arg_value);
                }
                // add the args
                json_object_set_value(method_object, "args", args_value);
                // add the return type
                if((*function)->type->returnType != NULL)
                    json_object_set_value(method_object, "returnType", ast_json_serializeDataTypeRecursive((*function)->type->returnType));
                else
                    json_object_set_null(method_object, "returnType");
                // add the method
                json_array_append_value(methods_array, method_value);
            }
            // add the methods
            json_object_set_value(root_object, "methods", methods_value);

            break;
        }
        case DT_STRUCT: {
            // category = struct
            json_object_set_string(root_object, "category", "struct");
            // add the fields
            JSON_Value *fields_value = json_value_init_array();
            JSON_Array *fields_array = json_value_get_array(fields_value);
            // iterate over the fields
            char *fieldName;
            uint32_t i;
            vec_foreach(&type->structType->attributeNames, fieldName, i) {
                    // we want to create an object {"name": <name>, "type": <type>}
                    JSON_Value *field_value = json_value_init_object();
                    JSON_Object *field_object = json_value_get_object(field_value);
                    // add the name
                    json_object_set_string(field_object, "name", fieldName);
                    // add the type
                    StructAttribute ** att = map_get(&type->structType->attributes, fieldName);
                    json_object_set_value(field_object, "type",
                                          ast_json_serializeDataTypeRecursive((*att)->type));
                    // add the field
                    json_array_append_value(fields_array, field_value);
                }
            // add the fields
            json_object_set_value(root_object, "fields", fields_value);

            break;
        }
        case DT_ENUM: {
            // category = enum
            json_object_set_string(root_object, "category", "enum");
            // add the fields
            JSON_Value *values_value = json_value_init_array();
            JSON_Array *values_array = json_value_get_array(values_value);
            // iterate over the fields
            char *valueName;
            uint32_t i;
            vec_foreach(&type->enumType->enumNames, valueName, i) {

                    // we want to create an object {"name": <name>, "type": <type>}
                    JSON_Value *value_value = json_value_init_object();
                    JSON_Object *value_object = json_value_get_object(value_value);
                    // add the name
                    json_object_set_string(value_object, "name", valueName);
                    // add the value
                    json_object_set_number(value_object, "value", i);
                    // add the field
                    json_array_append_value(values_array, value_value);
                }
            // add the fields
            json_object_set_value(root_object, "enums", values_value);

            break;
        }
        case DT_VARIANT: {
            // category = variant
            json_object_set_string(root_object, "category", "variant");
            // add the fields
            JSON_Value *variants_value = json_value_init_array();
            JSON_Array *variants_array = json_value_get_array(variants_value);
            // iterate over the fields
            char *variantName;
            uint32_t i;
            vec_foreach(&type->variantType->constructorNames, variantName, i) {
                    // we want to create an object {"name": <name>, "type": <type>}
                    JSON_Value *variant_value = json_value_init_object();
                    JSON_Object *variant_object = json_value_get_object(variant_value);
                    // add the name
                    json_object_set_string(variant_object, "name", variantName);
                    // add the type
                    VariantConstructor **variant = map_get(&type->variantType->constructors, variantName);
                    // each constructor has args
                    JSON_Value *args_value = json_value_init_array();
                    JSON_Array *args_array = json_value_get_array(args_value);
                    // iterate over the args
                    char *argName;
                    uint32_t j;
                    vec_foreach(&(*variant)->argNames, argName, j) {
                            VariantConstructorArgument **argType = map_get(&(*variant)->args, argName);
                            // we want to create an object {"name": <name>, "type": <type>}
                            JSON_Value *arg_value = json_value_init_object();
                            JSON_Object *arg_object = json_value_get_object(arg_value);
                            // add the name
                            json_object_set_string(arg_object, "name", argName);
                            // add the type
                            json_object_set_value(arg_object, "type",
                                                  ast_json_serializeDataTypeRecursive((*argType)->type));
                            // add the arg
                            json_array_append_value(args_array, arg_value);
                        }
                    // add the args
                    json_object_set_value(variant_object, "args", args_value);

                    // add the field
                    json_array_append_value(variants_array, variant_value);
                }
            // add the fields
            json_object_set_value(root_object, "variants", variants_value);

            break;
        }
        case DT_ARRAY: {
            // category = array
            json_object_set_string(root_object, "category", "array");
            // add the type
            json_object_set_value(root_object, "arrayOf",
                                  ast_json_serializeDataTypeRecursive(type->arrayType->arrayOf));
            // add the size
            json_object_set_number(root_object, "size", (double) type->arrayType->len);

            break;
        }
        case DT_FN: {
            // category = fn
            json_object_set_string(root_object, "category", "fn");
            // add the return type if it's not null, else write null
            if (type->fnType->returnType != NULL) {
                json_object_set_value(root_object, "returnType",
                                      ast_json_serializeDataTypeRecursive(type->fnType->returnType));
            } else {
                json_object_set_value(root_object, "returnType", json_value_init_null());
            }
            // add the args
            JSON_Value *args_value = json_value_init_array();
            JSON_Array *args_array = json_value_get_array(args_value);
            // iterate over the args
            char *argName;
            uint32_t i = 0;
            vec_foreach(&type->fnType->argNames, argName, i) {
                // we want to create an object {"name": <name>, "type": <type>}
                JSON_Value *arg_value = json_value_init_object();
                JSON_Object *arg_object = json_value_get_object(arg_value);
                // add the name
                json_object_set_string(arg_object, "name", argName);
                // add the type
                FnArgument ** arg = map_get(&type->fnType->args, argName);
                json_object_set_value(arg_object, "type",
                                      ast_json_serializeDataTypeRecursive((*arg)->type));
                // add isMutable
                json_object_set_boolean(arg_object, "isMutable", (*arg)->isMutable);

                // add the arg
                json_array_append_value(args_array, arg_value);
            }
            // add the args
            json_object_set_value(root_object, "args", args_value);
            break;
        }
        case DT_PTR:
            // category = ptr
            json_object_set_string(root_object, "category", "ptr");
            // add the type
            json_object_set_value(root_object, "ptrOf", ast_json_serializeDataTypeRecursive(type->ptrType->target));

            break;
        case DT_REFERENCE:
            // category = reference
            json_object_set_string(root_object, "category", "reference");
            // add the referenceTo name
            if(type->refType->ref != NULL) {
                json_object_set_string(root_object, "referenceTo", type->refType->ref->name);
                json_object_set_boolean(root_object, "isRefConcrete", 1);
            } else {
                // iterate over the pkg
                // prepare
                char *pkgName;
                uint32_t i;
                sds pkgNameSds = sdsempty();
                vec_foreach(&type->refType->pkg->ids, pkgName, i) {
                    pkgNameSds = sdscat(pkgNameSds, pkgName);
                    if(i != type->refType->pkg->ids.length - 1) {
                        pkgNameSds = sdscat(pkgNameSds, ".");
                    }
                }

                json_object_set_string(root_object, "referenceTo", pkgNameSds);
                json_object_set_boolean(root_object, "isRefConcrete", 0);
            }
            break;
        case DT_TYPE_JOIN: {
            // category = typeJoin
            json_object_set_string(root_object, "category", "typeJoin");
            // add the left type as left: <type>
            json_object_set_value(root_object, "left", ast_json_serializeDataTypeRecursive(type->joinType->left));
            // add the right type as right: <type>
            json_object_set_value(root_object, "right", ast_json_serializeDataTypeRecursive(type->joinType->right));
            break;
        }
        case DT_TYPE_UNION: {
            // category = typeJoin
            json_object_set_string(root_object, "category", "typeUnion");
            // add the left type as left: <type>
            json_object_set_value(root_object, "left", ast_json_serializeDataTypeRecursive(type->unionType->left));
            // add the right type as right: <type>
            json_object_set_value(root_object, "right", ast_json_serializeDataTypeRecursive(type->unionType->right));
            break;
        }
        case DT_PROCESS: {
            // category = process
            json_object_set_string(root_object, "category", "process");
            ProcessType * pt = type->processType;

            // add input type
            json_object_set_value(root_object, "input", ast_json_serializeDataTypeRecursive(pt->inputType));
            // add output type
            json_object_set_value(root_object, "output", ast_json_serializeDataTypeRecursive(pt->outputType));

            // add constructor args as constructor: [args]
            JSON_Value *args_value = json_value_init_array();
            JSON_Array *args_array = json_value_get_array(args_value);
            // iterate over the args
            char *argName;
            uint32_t i;
            vec_foreach(&pt->argNames, argName, i) {
                FnArgument ** arg = map_get(&pt->args, argName);
                // we want to create an object {"name": <name>, "type": <type>}
                JSON_Value *arg_value = json_value_init_object();
                JSON_Object *arg_object = json_value_get_object(arg_value);
                // add the name
                json_object_set_string(arg_object, "name", argName);
                // add the type
                json_object_set_value(arg_object, "type",
                                      ast_json_serializeDataTypeRecursive((*arg)->type));

                // add the arg
                json_array_append_value(args_array, arg_value);
            }
            // add the args
            json_object_set_value(root_object, "constructor", args_value);

            break;
        }
        case DT_UNRESOLVED:
            // category = unresolved
            json_object_set_string(root_object, "category", "unresolved");
            break;
    }

    return root_value;

}

char* literalTypeToString(LiteralType lt){
    switch(lt){
        case LT_FLOAT:
            return "float";
        case LT_STRING:
            return "string";
        case LT_INTEGER:
            return "int";
        case LT_BINARY_INT:
            return "binaryInt";
        case LT_OCTAL_INT:
            return "octalInt";
        case LT_HEX_INT:
            return "hexInt";
        case LT_DOUBLE:
            return "double";
        case LT_BOOLEAN:
            return "boolean";
        case LT_CHARACTER:
            return "character";
    }
}

JSON_Value* ast_json_serializeExprRecursive(Expr* expr) {
    // create the root object
    JSON_Value * root_value = json_value_init_object();
    JSON_Object * root_object = json_value_get_object(root_value);

    switch(expr->type){
        case ET_LITERAL: {
            // category = literal
            json_object_set_string(root_object, "category", "literal");
            json_object_set_string(root_object, "strValue", expr->literalExpr->value);
            json_object_set_string(root_object, "literalType", literalTypeToString(expr->literalExpr->type));

            break;
        }
        case ET_ELEMENT: {
            // category = element
            json_object_set_string(root_object, "category", "element");
            // add the name
            json_object_set_string(root_object, "name", expr->elementExpr->name);
            break;
        }
        case ET_THIS: {
            // category = this
            json_object_set_string(root_object, "category", "this");
        }
        case ET_ARRAY_CONSTRUCTION: {
            // category = arrayConstruction
            json_object_set_string(root_object, "category", "arrayConstruction");
            // add the values
            JSON_Value * values_value = json_value_init_array();
            JSON_Array * values_array = json_value_get_array(values_value);
            // iterate over the values
            int i; Expr * arg;
            ArrayConstructionExpr * arrayConstruction = expr->arrayConstructionExpr;
            vec_foreach(&arrayConstruction->args, arg, i) {
                JSON_Value * v = ast_json_serializeExprRecursive(arg);
                json_array_append_value(values_array, v);
            }
            // add the values
            json_object_set_value(root_object, "values", values_value);

            break;
        }
        case ET_NAMED_STRUCT_CONSTRUCTION: {
            // category = namedStructConstruction
            json_object_set_string(root_object, "category", "namedStructConstruction");
            // create fields array
            JSON_Value *fields_value = json_value_init_array();
            JSON_Array *fields_array = json_value_get_array(fields_value);
            // iterate over the fields
            int i;
            char *argName;
            NamedStructConstructionExpr *namedStructConstruction = expr->namedStructConstructionExpr;
            vec_foreach(&namedStructConstruction->argNames, argName, i) {
                Expr** arg = map_get(&namedStructConstruction->args, argName);

                // for each arg, serialize it and add it to the fields object
                JSON_Value *arg_value = json_value_init_object();
                JSON_Object *arg_object = json_value_get_object(arg_value);

                // set the name
                json_object_set_string(arg_object, "name", argName);
                // set the value
                json_object_set_value(arg_object, "value", ast_json_serializeExprRecursive(*arg));
                // add to the array of fields
                json_array_append_value(fields_array, arg_value);
            }
            // add the fields
            json_object_set_value(root_object, "fields", fields_value);

            break;
        }
        case ET_UNNAMED_STRUCT_CONSTRUCTION: {
            // category = unnamedStructConstruction
            json_object_set_string(root_object, "category", "unnamedStructConstruction");
            // create an array to store values
            JSON_Value *values_value = json_value_init_array();
            JSON_Array *values_array = json_value_get_array(values_value);
            // iterate over the values
            int i;
            Expr *arg;
            UnnamedStructConstructionExpr *unnamedStructConstruction = expr->unnamedStructConstructionExpr;
            vec_foreach(&unnamedStructConstruction->args, arg, i) {
                    JSON_Value *v = ast_json_serializeExprRecursive(arg);
                    json_array_append_value(values_array, v);
                }
            // add the values
            json_object_set_value(root_object, "values", values_value);

            break;
        }
        case ET_NEW: {
            // category = new
            json_object_set_string(root_object, "category", "new");
            // add the type
            json_object_set_value(root_object, "type", ast_json_serializeDataTypeRecursive(expr->newExpr->type));
            // add the args
            JSON_Value *args_value = json_value_init_array();
            JSON_Array *args_array = json_value_get_array(args_value);
            // iterate over the args
            int i;
            Expr *arg;
            vec_foreach(&expr->newExpr->args, arg, i) {
                    JSON_Value *v = ast_json_serializeExprRecursive(arg);
                    json_array_append_value(args_array, v);
                }
            // add the args
            json_object_set_value(root_object, "args", args_value);

            break;
        }
        case ET_CALL: {
            // category = call
            json_object_set_string(root_object, "category", "call");
            // add if it has generics
            json_object_set_boolean(root_object, "hasGenerics", expr->callExpr->hasGenerics);
            // add the generics or empty array
            JSON_Value * generics_value = json_value_init_array();
            JSON_Array * generics_array = json_value_get_array(generics_value);
            // iterate over the generics
            int j; DataType * generic;
            vec_foreach(&expr->callExpr->generics, generic, j) {
                JSON_Value * v = ast_json_serializeDataTypeRecursive(generic);
                json_array_append_value(generics_array, v);
            }
            // add the generics
            json_object_set_value(root_object, "generics", generics_value);

            // add the lhs
            json_object_set_value(root_object, "lhs", ast_json_serializeExprRecursive(expr->callExpr->lhs));
            // add the args
            JSON_Value * args_value = json_value_init_array();
            JSON_Array * args_array = json_value_get_array(args_value);
            // iterate over the args
            int i; Expr * arg;
            vec_foreach(&expr->callExpr->args, arg, i) {
                JSON_Value * v = ast_json_serializeExprRecursive(arg);
                json_array_append_value(args_array, v);
            }
            // add the args
            json_object_set_value(root_object, "args", args_value);

            break;
        }
        case ET_MEMBER_ACCESS: {
            // category = memberAccess
            json_object_set_string(root_object, "category", "memberAccess");
            // add the lhs
            json_object_set_value(root_object, "lhs", ast_json_serializeExprRecursive(expr->memberAccessExpr->lhs));
            // add the rhs
            json_object_set_value(root_object, "rhs", ast_json_serializeExprRecursive(expr->memberAccessExpr->rhs));

            break;
        }
        case ET_INDEX_ACCESS: {
            // category = indexAccess
            json_object_set_string(root_object, "category", "indexAccess");
            // add the lhs
            json_object_set_value(root_object, "lhs", ast_json_serializeExprRecursive(expr->indexAccessExpr->expr));
            // add the indexes
            JSON_Value * indexes_value = json_value_init_array();
            JSON_Array * indexes_array = json_value_get_array(indexes_value);
            // iterate over the indexes
            int i; Expr * index;
            vec_foreach(&expr->indexAccessExpr->indexes, index, i) {
                JSON_Value * v = ast_json_serializeExprRecursive(index);
                json_array_append_value(indexes_array, v);
            }
            // add the indexes
            json_object_set_value(root_object, "indexes", indexes_value);

            break;
        }
        case ET_CAST: {
            // category = cast
            json_object_set_string(root_object, "category", "cast");
            // add the type
            json_object_set_value(root_object, "type", ast_json_serializeDataTypeRecursive(expr->castExpr->type));
            // add the expr
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(expr->castExpr->expr));

            break;
        }
        case ET_INSTANCE_CHECK: {
            // category = instanceCheck
            json_object_set_string(root_object, "category", "instanceCheck");
            // add the type
            json_object_set_value(root_object, "type", ast_json_serializeDataTypeRecursive(expr->instanceCheckExpr->type));
            // add the expr
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(expr->instanceCheckExpr->expr));

            break;
        }
        case ET_UNARY: {
            // category = unary
            json_object_set_string(root_object, "category", "unary");
            // add the op
            json_object_set_string(root_object, "op", ast_stringifyUnaryExprType(expr->unaryExpr->type));
            // add the expr
            json_object_set_value(root_object, "uhs", ast_json_serializeExprRecursive(expr->unaryExpr->uhs));

            break;
        }
        case ET_BINARY: {
            // category = binary
            json_object_set_string(root_object, "category", "binary");
            // add the op
            json_object_set_string(root_object, "op", ast_stringifyBinaryExprType(expr->binaryExpr->type));
            // add the lhs
            json_object_set_value(root_object, "lhs", ast_json_serializeExprRecursive(expr->binaryExpr->lhs));
            // add the rhs
            json_object_set_value(root_object, "rhs", ast_json_serializeExprRecursive(expr->binaryExpr->rhs));
            break;
        }
        case ET_IF_ELSE: {
            // category = ifElse
            json_object_set_string(root_object, "category", "ifElse");
            // add the condition
            json_object_set_value(root_object, "condition", ast_json_serializeExprRecursive(expr->ifElseExpr->condition));
            // add the thenExpr
            json_object_set_value(root_object, "ifExpr", ast_json_serializeExprRecursive(expr->ifElseExpr->ifExpr));
            // add the elseExpr
            json_object_set_value(root_object, "elseExpr", ast_json_serializeExprRecursive(expr->ifElseExpr->elseExpr));

            break;
        }
        case ET_MATCH: {
            // category = match
            json_object_set_string(root_object, "category", "match");
            // add the expr
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(expr->matchExpr->expr));
            // add the cases
            JSON_Value * cases_value = json_value_init_array();
            JSON_Array * cases_array = json_value_get_array(cases_value);
            // iterate over the cases
            int i; CaseExpr * matchCase;
            vec_foreach(&expr->matchExpr->cases, matchCase, i) {
                // we create a new object which will contain condition and action
                JSON_Value * case_value = json_value_init_object();
                JSON_Object * case_object = json_value_get_object(case_value);
                // add the condition
                JSON_Value * v = ast_json_serializeExprRecursive(matchCase->condition);
                json_object_set_value(case_object, "condition", v);
                // add the action
                v = ast_json_serializeExprRecursive(matchCase->expr);
                json_object_set_value(case_object, "expr", v);
                // add the case to the cases array
                json_array_append_value(cases_array, case_value);

            }
            // add the cases
            json_object_set_value(root_object, "cases", cases_value);

            break;
        }
        case ET_LET: {
            LetExpr * let = expr->letExpr;
            // category = let
            json_object_set_string(root_object, "category", "let");
            // a let is composed of assignment groups
            // for example let {a, b} = c, d = e, here a,b are one group who is equal to c, while d is another
            // so we create an array of assignment groups
            JSON_Value * assignmentGroups_value = json_value_init_array();
            JSON_Array * assignmentGroups_array = json_value_get_array(assignmentGroups_value);
            // iterate over the assignment groups
            uint32_t i; LetExprDecl * letDecl;
            vec_foreach(&let->letList, letDecl, i) {
                // create a group
                JSON_Value * assignmentGroup_value = json_value_init_object();
                JSON_Object * assignmentGroup_object = json_value_get_object(assignmentGroup_value);

                // each group object will contain an array of variables
                JSON_Value * variables_value = json_value_init_array();
                JSON_Array * variables_array = json_value_get_array(variables_value);

                // iterate over the variables
                int j; char * var;
                vec_foreach(&letDecl->variableNames, var, j) {
                    // get the variable var from the map
                    FnArgument **varDecl = map_get(&letDecl->variables, var);
                    // create a variable object
                    JSON_Value * variable_value = json_value_init_object();
                    JSON_Object * variable_object = json_value_get_object(variable_value);
                    // add the name
                    json_object_set_string(variable_object, "name", var);
                    // add the type if it's not null, otherwise write null
                    if ((*varDecl)->type != NULL)
                        json_object_set_value(variable_object, "type", ast_json_serializeDataTypeRecursive((*varDecl)->type));
                    else
                        json_object_set_string(variable_object, "type", "null");
                    // add the variable to the variables array
                    json_array_append_value(variables_array, variable_value);
                }
                // add the variables array to the group object
                json_object_set_value(assignmentGroup_object, "variables", variables_value);
                // add the assigned value of that group
                json_object_set_string(assignmentGroup_object, "initializerType", letDecl->initializerType==LIT_STRUCT_DECONSTRUCTION?"structDeconstruction":(letDecl->initializerType==LIT_NONE?"none":"arrayDestruction"));
                json_object_set_value(assignmentGroup_object, "initializer", ast_json_serializeExprRecursive(letDecl->initializer));
                // add the group to the groups array
                json_array_append_value(assignmentGroups_array, assignmentGroup_value);
            }
            // add the groups array to the root object
            json_object_set_value(root_object, "assignmentGroups", assignmentGroups_value);
            // add "in" expression
            json_object_set_value(root_object, "in", ast_json_serializeExprRecursive(let->inExpr));

            break;
        }
        case ET_LAMBDA: {
            LambdaExpr *lambda = expr->lambdaExpr;
            // category = lambda
            json_object_set_string(root_object, "category", "lambda");
            // add args
            JSON_Value *args_value = json_value_init_array();
            JSON_Array *args_array = json_value_get_array(args_value);

            json_object_set_string(root_object, "bodyType",lambda->bodyType == FBT_BLOCK?"block":"expr");

            // iterate over the args
            int i;
            char *argName;
            vec_foreach(&lambda->header->type->argNames, argName, i) {
                FnArgument ** arg = map_get(&lambda->header->type->args, argName);
                // create new obj to hold arg name and type
                JSON_Value *arg_value = json_value_init_object();
                JSON_Object *arg_object = json_value_get_object(arg_value);
                // add the name
                json_object_set_string(arg_object, "name", argName);
                // add isMutable
                json_object_set_boolean(arg_object, "isMutable", (*arg)->isMutable);
                // add the type
                json_object_set_value(arg_object, "type", ast_json_serializeDataTypeRecursive((*arg)->type));

                // add the arg to the args array
                json_array_append_value(args_array, arg_value);

            }
            // add the args array to the root object
            json_object_set_value(root_object, "args", args_value);
            // add the body
            if(lambda->bodyType == FBT_EXPR){
                json_object_set_value(root_object, "body", ast_json_serializeExprRecursive(lambda->expr));
            }
            else {
                // add statement body
                json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(lambda->block));
            }

        // TODO: print lambda body
        break;
        }
        case ET_UNSAFE: {
            // category = unsafe
            json_object_set_string(root_object, "category", "unsafe");
            // add the unsafe expression
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(expr->unsafeExpr->expr));
            break;
        }
        case ET_SYNC: {
            // category = unsafe
            json_object_set_string(root_object, "category", "sync");
            // add the unsafe expression
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(expr->syncExpr->expr));
            break;
        }
        case ET_SPAWN: {
            // category = spawn
            json_object_set_string(root_object, "category", "spawn");
            // add the callback expr if exists else null
            if(expr->spawnExpr->callback != NULL)
                json_object_set_value(root_object, "callback", ast_json_serializeExprRecursive(expr->spawnExpr->callback));
            else
                json_object_set_null(root_object, "callback");
            // add the process
            json_object_set_value(root_object, "process", ast_json_serializeExprRecursive(expr->spawnExpr->expr));
        }
        case ET_EMIT: {
            // category = emit
            json_object_set_string(root_object, "category", "emit");
            // add the process if not null
            if(expr->emitExpr->process != NULL)
                json_object_set_value(root_object, "process", ast_json_serializeExprRecursive(expr->emitExpr->process));
            else
                json_object_set_null(root_object, "process");
            // add the message
            json_object_set_value(root_object, "message", ast_json_serializeExprRecursive(expr->emitExpr->msg));

            break;
        }
        case ET_WILDCARD:
            break;
    }
    return root_value;
}

JSON_Value* ast_json_serializeStatementRecursive(Statement* stmt){
    // create base object
    JSON_Value * root_value = json_value_init_object();
    JSON_Object * root_object = json_value_get_object(root_value);
    switch(stmt->type) {

        case ST_EXPR: {
            // category = expr
            json_object_set_string(root_object, "category", "expr");
            // add the expression
            json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(stmt->expr->expr));

            break;
        }
        case ST_VAR_DECL: {
            VarDeclStatement* varDecl = stmt->varDecl;
            json_object_set_string(root_object, "category", "let");
            // a let is composed of assignment groups
            // for example let {a, b} = c, d = e, here a,b are one group who is equal to c, while d is another
            // so we create an array of assignment groups
            JSON_Value * assignmentGroups_value = json_value_init_array();
            JSON_Array * assignmentGroups_array = json_value_get_array(assignmentGroups_value);
            // iterate over the assignment groups
            uint32_t i; LetExprDecl * letDecl;
            vec_foreach(&varDecl->letList, letDecl, i) {
                // create a group
                JSON_Value * assignmentGroup_value = json_value_init_object();
                JSON_Object * assignmentGroup_object = json_value_get_object(assignmentGroup_value);

                // each group object will contain an array of variables
                JSON_Value * variables_value = json_value_init_array();
                JSON_Array * variables_array = json_value_get_array(variables_value);

                // iterate over the variables
                int j; char * var;
                vec_foreach(&letDecl->variableNames, var, j) {
                        // get the variable var from the map
                        FnArgument **v = map_get(&letDecl->variables, var);
                        // create a variable object
                        JSON_Value * variable_value = json_value_init_object();
                        JSON_Object * variable_object = json_value_get_object(variable_value);
                        // add the name
                        json_object_set_string(variable_object, "name", var);
                        // add the type if it's not null, otherwise write null
                        if ((*v)->type != NULL)
                            json_object_set_value(variable_object, "type", ast_json_serializeDataTypeRecursive((*v)->type));
                        else
                            json_object_set_string(variable_object, "type", "null");
                        // add the variable to the variables array
                        json_array_append_value(variables_array, variable_value);
                    }
                // add the variables array to the group object
                json_object_set_value(assignmentGroup_object, "variables", variables_value);
                // add the assigned value of that group
                json_object_set_string(assignmentGroup_object, "initializerType", letDecl->initializerType==LIT_STRUCT_DECONSTRUCTION?"structDeconstruction":(letDecl->initializerType==LIT_NONE?"none":"arrayDestruction"));
                json_object_set_value(assignmentGroup_object, "initializer", ast_json_serializeExprRecursive(letDecl->initializer));
                // add the group to the groups array
                json_array_append_value(assignmentGroups_array, assignmentGroup_value);
            }
            // add the groups array to the root object
            json_object_set_value(root_object, "assignmentGroups", assignmentGroups_value);
            // add "in" expression

            break;
        }
        case ST_FN_DECL: {
            // category = fnDecl
            json_object_set_string(root_object, "category", "fnDecl");
            // add body type
            json_object_set_string(root_object, "bodyType", stmt->fnDecl->bodyType==FBT_EXPR?"expr":"block");
            // add the name
            json_object_set_string(root_object, "name", stmt->fnDecl->header->name);
            // add the return type if exists, else set value to null
            if(stmt->fnDecl->header->type->returnType != NULL)
                json_object_set_value(root_object, "returnType", ast_json_serializeDataTypeRecursive(stmt->fnDecl->header->type->returnType));
            else
                json_object_set_null(root_object, "returnType");
            // add function isGeneric
            json_object_set_boolean(root_object, "hasGeneric", stmt->fnDecl->hasGeneric);
            // add generic params in an array
            JSON_Value * genericParams_value = json_value_init_array();
            JSON_Array * genericParams_array = json_value_get_array(genericParams_value);
            // iterate over the generic params
            uint32_t j = 0; GenericParam * genericParam;
            vec_foreach(&stmt->fnDecl->genericParams, genericParam, j) {
                // create a generic param object
                JSON_Value * genericParam_value = json_value_init_object();
                JSON_Object * genericParam_object = json_value_get_object(genericParam_value);
                // add the name
                json_object_set_string(genericParam_object, "name", genericParam->name);
                // add the requirements or null if it's null
                if (genericParam->constraint != NULL)
                    json_object_set_value(genericParam_object, "supertype", ast_json_serializeDataTypeRecursive(genericParam->constraint));
                else
                    json_object_set_null(genericParam_object, "requirements");


                // add the generic param to the generic params array
                json_array_append_value(genericParams_array, genericParam_value);
            }
            // add the generic params array to the root object
            json_object_set_value(root_object, "genericParams", genericParams_value);


            // add the args
            // create an array of args
            JSON_Value * args_value = json_value_init_array();
            JSON_Array * args_array = json_value_get_array(args_value);
            // iterate over the args
            uint32_t i; char * argName;
            vec_foreach(&stmt->fnDecl->header->type->argNames, argName, i) {
                FnArgument** arg = map_get(&stmt->fnDecl->header->type->args, argName);
                // create an arg object
                JSON_Value * arg_value = json_value_init_object();
                JSON_Object * arg_object = json_value_get_object(arg_value);
                // add the name
                json_object_set_string(arg_object, "name", argName);
                // add the type
                json_object_set_value(arg_object, "type", ast_json_serializeDataTypeRecursive((*arg)->type));
                // add the arg to the args array
                json_array_append_value(args_array, arg_value);
                // add isMutable
                json_object_set_boolean(arg_object, "isMutable", (*arg)->isMutable);
                // add to array
                json_array_append_value(args_array, arg_value);
            }
            // add the args array to the root object
            json_object_set_value(root_object, "args", args_value);

            // if body type is expression we set expr
            if (stmt->fnDecl->bodyType == FBT_EXPR)
                json_object_set_value(root_object, "expr", ast_json_serializeExprRecursive(stmt->fnDecl->expr));
            else
                json_object_set_value(root_object, "block", ast_json_serializeStatementRecursive(stmt->fnDecl->block));

            break;
        }
        case ST_BLOCK: {
            // category = block
            json_object_set_string(root_object, "category", "block");
            // create an array of statements
            JSON_Value * statements_value = json_value_init_array();
            JSON_Array * statements_array = json_value_get_array(statements_value);
            // iterate over the statements
            uint32_t i; Statement * statement;
            vec_foreach(&stmt->blockStmt->stmts, statement, i) {
                // add the statement to the statements array
                json_array_append_value(statements_array, ast_json_serializeStatementRecursive(statement));
            }
            // add the statements array to the root object
            json_object_set_value(root_object, "statements", statements_value);

            break;
        }
        case ST_IF_CHAIN: {
            // category = ifChain
            json_object_set_string(root_object, "category", "ifChain");
            // we create an array of if conditions and their bodies per object
            JSON_Value * ifConditions_value = json_value_init_array();
            JSON_Array * ifConditions_array = json_value_get_array(ifConditions_value);
            // iterate over the if conditions
            uint32_t i; Expr * ifCondition;
            vec_foreach(&stmt->ifChain->conditions, ifCondition, i) {
                // create an if condition object
                JSON_Value * ifCondition_value = json_value_init_object();
                JSON_Object * ifCondition_object = json_value_get_object(ifCondition_value);
                // add the condition
                json_object_set_value(ifCondition_object, "condition", ast_json_serializeExprRecursive(ifCondition));
                // add the body
                json_object_set_value(ifCondition_object, "body", ast_json_serializeStatementRecursive(stmt->ifChain->blocks.data[i]));
                // add the if condition object to the if conditions array
                json_array_append_value(ifConditions_array, ifCondition_value);
            }
            // add the if conditions array to the root object
            json_object_set_value(root_object, "chain", ifConditions_value);
            // add else as null as its null or block
            json_object_set_value(root_object, "else", stmt->ifChain->elseBlock==NULL?json_value_init_null():ast_json_serializeStatementRecursive(stmt->ifChain->elseBlock));

            break;
        }
        case ST_MATCH: {
            // category = match
            json_object_set_string(root_object, "category", "match");
            // create array to store match cases
            JSON_Value * matchCases_value = json_value_init_array();
            JSON_Array * matchCases_array = json_value_get_array(matchCases_value);
            // iterate through CaseStatement
            uint32_t i; CaseStatement * caseStmt;
            vec_foreach(&stmt->match->cases, caseStmt, i) {
                // create a match case object
                JSON_Value * matchCase_value = json_value_init_object();
                JSON_Object * matchCase_object = json_value_get_object(matchCase_value);
                // add the condition
                json_object_set_value(matchCase_object, "condition", ast_json_serializeExprRecursive(caseStmt->condition));
                // add the body
                json_object_set_value(matchCase_object, "body", ast_json_serializeStatementRecursive(caseStmt->block));
                // add the match case object to the match cases array
                json_array_append_value(matchCases_array, matchCase_value);
            }
            // add the match cases array to the root object
            json_object_set_value(root_object, "cases", matchCases_value);
            // add else as null as its null or block
            json_object_set_value(root_object, "else", stmt->match->elseBlock==NULL?json_value_init_null():ast_json_serializeStatementRecursive(stmt->match->elseBlock));

            break;
        }

        case ST_WHILE: {
            // category = while
            json_object_set_string(root_object, "category", "while");
            // add the condition
            json_object_set_value(root_object, "condition", ast_json_serializeExprRecursive(stmt->whileLoop->condition));
            // add the body
            json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(stmt->whileLoop->block));

            break;
        }
        case ST_DO_WHILE: {
            // category = doWhile
            json_object_set_string(root_object, "category", "doWhile");
            // add the condition
            json_object_set_value(root_object, "condition", ast_json_serializeExprRecursive(stmt->doWhileLoop->condition));
            // add the body
            json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(stmt->doWhileLoop->block));

            break;
        }
        case ST_FOR: {
            // category = for
            json_object_set_string(root_object, "category", "for");
            // add the init
            json_object_set_value(root_object, "init", ast_json_serializeStatementRecursive(stmt->forLoop->initializer));
            // add the condition
            json_object_set_value(root_object, "condition", ast_json_serializeExprRecursive(stmt->forLoop->condition));
            // add the increment as an array
            JSON_Value * increments_value = json_value_init_array();
            JSON_Array * increments_array = json_value_get_array(increments_value);
            // iterate over the increments
            uint32_t i; Expr * increment;
            vec_foreach(&stmt->forLoop->increments, increment, i) {
                // add the increment to the increments array
                json_array_append_value(increments_array, ast_json_serializeExprRecursive(increment));
            }
            // add the increments array to the root object
            json_object_set_value(root_object, "increments", increments_value);
            // add the body
            json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(stmt->forLoop->block));

            break;
        }
        case ST_FOREACH:
            // throw not implemented error
            ASSERT(0, "Foreach not implemented yet");
            break;
        case ST_CONTINUE: {
            // category = continue
            json_object_set_string(root_object, "category", "continue");
            break;
        }
        case ST_RETURN: {
            // category = return
            json_object_set_string(root_object, "category", "return");
            // add the return value if it exists else set it to null
            json_object_set_value(root_object, "value", stmt->returnStmt->expr==NULL?json_value_init_null():ast_json_serializeExprRecursive(stmt->returnStmt->expr));
            break;
        }
        case ST_BREAK: {
            // category = break
            json_object_set_string(root_object, "category", "break");
            break;
        }
        case ST_UNSAFE: {
            // category = unsafe
            json_object_set_string(root_object, "category", "unsafe");
            // add the unsafe block
            json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(stmt->unsafeStmt->block));
            break;
        }
        case ST_SYNC: {
            // category = unsafe
            json_object_set_string(root_object, "category", "sync");
            // add the unsafe block
            json_object_set_value(root_object, "body", ast_json_serializeStatementRecursive(stmt->unsafeStmt->block));
            break;
        }
        case ST_SPAWN: {
            // category = spawn
            json_object_set_string(root_object, "category", "spawn");
            break;
        }
        case ST_EMIT:
            break;
    }
    return root_value;
}

JSON_Value* ast_json_serializeExternDeclRecursive(ExternDecl* decl) {
    // create the root object
    JSON_Value * root_value = json_value_init_object();
    JSON_Object * root_object = json_value_get_object(root_value);
    // add the name
    json_object_set_string(root_object, "name", decl->name);
    // add the linkage and set it to C by default
    json_object_set_string(root_object, "linkage", "C");
    // add the interface methods
    JSON_Value* methods_value = json_value_init_array();
    JSON_Array* methods_array = json_value_get_array(methods_value);
    // iterate over the methods
    char * methodName; uint32_t i;
    vec_foreach(&decl->methodNames, methodName, i){
            // we want to create an object {"name": <name>, "args": [<args>], "returnType": <returnType>}
            JSON_Value* method_value = json_value_init_object();
            JSON_Object* method_object = json_value_get_object(method_value);
            // add the name
            json_object_set_string(method_object, "name", methodName);
            // add the args
            JSON_Value* args_value = json_value_init_array();
            JSON_Array* args_array = json_value_get_array(args_value);
            // iterate over the args
            FnHeader ** function = map_get(&decl->methods, methodName);
            uint32_t j; char* argName;
            vec_foreach(&(*function)->type->argNames, argName, j) {
                    FnArgument ** argType = map_get(&(*function)->type->args, argName);
                    // we want to create an object {"name": <name>, "type": <type>}
                    JSON_Value* arg_value = json_value_init_object();
                    JSON_Object* arg_object = json_value_get_object(arg_value);
                    // add the name
                    json_object_set_string(arg_object, "name", argName);
                    // add the type
                    json_object_set_value(arg_object, "type", ast_json_serializeDataTypeRecursive((*argType)->type));
                    // add the arg
                    json_array_append_value(args_array, arg_value);
                }
            // add the args
            json_object_set_value(method_object, "args", args_value);
            // add the return type
            if((*function)->type->returnType != NULL)
                json_object_set_value(method_object, "returnType", ast_json_serializeDataTypeRecursive((*function)->type->returnType));
            else
                json_object_set_null(method_object, "returnType");
            // add the method
            json_array_append_value(methods_array, method_value);
        }
    // add the methods
    json_object_set_value(root_object, "methods", methods_value);
    return root_value;
}

char* ast_json_serializeDataType(DataType* type) {
    JSON_Value * root_val = ast_json_serializeDataTypeRecursive(type);
    return json_serialize_to_string(root_val);
}

char* ast_json_serializeExpr(Expr* expr) {
    JSON_Value * root_val = ast_json_serializeExprRecursive(expr);
    return json_serialize_to_string(root_val);
}

char* ast_json_serializeStatement(Statement* stmt) {
    JSON_Value * root_val = ast_json_serializeStatementRecursive(stmt);
    return json_serialize_to_string(root_val);
}

char* ast_json_serializeExternDecl(ExternDecl* decl){
    JSON_Value * root_val = ast_json_serializeExternDeclRecursive(decl);
    return json_serialize_to_string(root_val);
}