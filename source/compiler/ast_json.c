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
                // set the constraint
                json_object_set_value(generic_object, "constraint", ast_json_serializeDataTypeRecursive(val->constraint));
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
        case DT_CLASS:
            // throw not implemented
            ASSERT(0==1, "Not implemented");
            break;
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
        case DT_VARIANT:
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
                VariantConstructor ** variant = map_get(&type->variantType->constructors, variantName);
                // each constructor has args
                JSON_Value* args_value = json_value_init_array();
                JSON_Array* args_array = json_value_get_array(args_value);
                // iterate over the args
                char* argName;
                uint32_t j;
                vec_foreach(&(*variant)->argNames, argName, j) {
                    VariantConstructorArgument ** argType = map_get(&(*variant)->args, argName);
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
                json_object_set_value(variant_object, "args", args_value);

                // add the field
                json_array_append_value(variants_array, variant_value);
            }
            // add the fields
            json_object_set_value(root_object, "variants", variants_value);

            break;
        case DT_ARRAY:
            // category = array
            json_object_set_string(root_object, "category", "array");
            // add the type
            json_object_set_value(root_object, "arrayOf", ast_json_serializeDataTypeRecursive(type->arrayType->arrayOf));
            // add the size
            json_object_set_number(root_object, "size", (double)type->arrayType->len);

            break;
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
            uint32_t i;
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
        case DT_UNRESOLVED:
            // category = unresolved
            json_object_set_string(root_object, "category", "unresolved");
            break;
    }

    return root_value;

}

char* ast_json_serializeDataType(DataType* type) {
    JSON_Value * root_val = ast_json_serializeDataTypeRecursive(type);
    return json_serialize_to_string(root_val);
}