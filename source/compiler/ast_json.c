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
        case ET_LAMBDA:
            break;
    }
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