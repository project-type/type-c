//
// Created by praisethemoon on 29.04.23.
//

#include "tokens.h"

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_ANY:
            return "any";
        /*case TOK_ASYNC:
            return "async";
        case TOK_AWAIT:
            return "await";*/
        case TOK_SYNC:
            return "sync";
        case TOK_TYPE_CONVERSION:
            return "as";
        case TOK_BOOLEAN:
            return "bool";
        case TOK_BREAK:
            return "break";
        case TOK_CASE:
            return "case";
        case TOK_CLASS:
            return "class";
        case TOK_CONTINUE:
            return "continue";
        case TOK_MUT:
            return "mut";
        case TOK_VARIANT:
            return "variant";
        //case TOK_DELETE:
        //    return "delete";
        case TOK_DO:
            return "do";
        case TOK_ELSE:
            return "else";
        case TOK_ENUM:
            return "enum";
        case TOK_EXTERN:
            return "extern";
        case TOK_FALSE:
            return "false";
        case TOK_FROM:
            return "from";
        case TOK_FOR:
            return "for";
        case TOK_FOREACH:
            return "foreach";
        case TOK_FN:
            return "fn";
        case TOK_IF:
            return "if";
        case TOK_IMPORT:
            return "import";
        case TOK_IN:
            return "in";
        case TOK_IS:
            return "is";
        case TOK_INTERFACE:
            return "interface";
        case TOK_I8:
            return "i8";
        case TOK_I16:
            return "i16";
        case TOK_I32:
            return "i32";
        case TOK_I64:
            return "i64";
        case TOK_U8:
            return "u8";
        case TOK_U16:
            return "u16";
        case TOK_U32:
            return "u32";
        case TOK_U64:
            return "u64";
        case TOK_F32:
            return "f32";
        case TOK_F64:
            return "f64";
        case TOK_STRING:
            return "string";
        case TOK_CHAR:
            return "char";
        case TOK_LET:
            return "let";
        case TOK_NEW:
            return "new";
        case TOK_NULL:
            return "null";
        case TOK_UNSAFE:
            return "unsafe";
        case TOK_PTR:
            return "ptr";
        case TOK_PROCESS:
            return "process";
        case TOK_PROCESS_LINK:
            return "::";
        case TOK_EMIT:
            return "emit";
        case TOK_SPAWN:
            return "spawn";
        case TOK_RETURN:
            return "return";
        //case TOK_SIZEOF:
        //    return "sizeof";
        //case TOK_SUPER:
        //    return "super";
        case TOK_THIS:
            return "this";
        //case TOK_STATIC:
        //    return "static";
        case TOK_STRUCT:
            return "struct";
        case TOK_MATCH:
            return "switch";
        case TOK_TRUE:
            return "true";
        case TOK_TYPE:
            return "type";
        case TOK_VEC:
            return "vec";
        case TOK_VOID:
            return "void";
        case TOK_WHILE:
            return "while";
        case TOK_SEMICOLON:
            return ";";
        case TOK_COLON:
            return ":";
        case TOK_COMMA:
            return ",";
        case TOK_DOT:
            return ".";
        case TOK_DOTDOTDOT:
            return "...";
        case TOK_LPAREN:
            return "(";
        case TOK_RPAREN:
            return ")";
        case TOK_LBRACKET:
            return "[";
        case TOK_RBRACKET:
            return "]";
        case TOK_LBRACE:
            return "{";
        case TOK_RBRACE:
            return "}";
        case TOK_NULLABLE:
            return "?";
        case TOK_NOT:
            return "!";
        case TOK_EQUAL:
            return "=";
        case TOK_LOGICAL_OR:
            return "||";
        case TOK_LOGICAL_AND:
            return "&&";
        case TOK_NOT_EQUAL:
            return "!=";
        case TOK_DENULL:
            return "!!";
        case TOK_LESS:
            return "<";
        case TOK_LESS_EQUAL:
            return "<=";
        case TOK_GREATER:
            return ">";
        case TOK_GREATER_EQUAL:
            return ">=";
        case TOK_PLUS:
            return "+";
        case TOK_PLUS_EQUAL:
            return "+=";
        case TOK_MINUS:
            return "-";
        case TOK_MINUS_EQUAL:
            return "-=";       // -=
        case TOK_STAR:
            return "*";
        case TOK_STAR_EQUAL:
            return "*=";
        case TOK_DIV:
            return "/";
        case TOK_DIV_EQUAL:
            return "/=";         // /=
        case TOK_PERCENT:
            return "%";           // %
        case TOK_BITWISE_NOT:
            return "~";       // ~
        case TOK_BITWISE_OR:
            return "|";        // |
        case TOK_BITWISE_XOR:
            return "^";       // ^
        case TOK_BITWISE_AND:
            return "&";       // &
        case TOK_LEFT_SHIFT:
            return "<<";        // <<
        case TOK_RIGHT_SHIFT:
            return ">>";       // >>
        case TOK_EQUAL_EQUAL:
            return "==";       // ==
        case TOK_INCREMENT:
            return "++";         // ++
        case TOK_DECREMENT:
            return "--";         // --
        case TOK_FN_RETURN_TYPE:
            return "->";         // ->
        case TOK_WILDCARD:
            return "_";          // _
            // Literals.
        case TOK_IDENTIFIER:
            return "Identifier";        // variable or function name
        case TOK_STRING_VAL:
            return "string literal";            // string literal  (double quotes)
        case TOK_CHAR_VAL:
            return "character literal";              // single character (single quote)
        case TOK_INT:
            return "int";
        case TOK_BINARY_INT:
            return "bin int";        // 0b[01]+
        case TOK_HEX_INT:
            return "hex int";           // 0x[0-9A-F]+
        case TOK_OCT_INT:
            return "oct int";           // 0o[0-7]+
        case TOK_FLOAT:
            return "float";             //
        case TOK_DOUBLE:
            return "double";
        case TOK_EOF:
            return "EOF";
        default:
            return "unknown";
    }
}