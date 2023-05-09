//
// Created by praisethemoon on 28.04.23.
//

#ifndef TYPE_C_TOKENS_H
#define TYPE_C_TOKENS_H

typedef enum {
    // Keywords.
    TOK_ANY,               // any
    //TOK_ASYNC,           // async
    //TOK_AWAIT,           // await
    TOK_SYNC,              // sync
    TOK_TYPE_CONVERSION,   // as
    TOK_BREAK,             // break
    TOK_CASE,              // case
    TOK_CLASS,             // class
    TOK_CONTINUE,          // continue
    TOK_MUT,               // mut
    TOK_VARIANT,           // variant
    //TOK_DELETE,          // delete
    TOK_DO,                // do
    TOK_ELSE,              // else
    TOK_ENUM,              // enum
    TOK_EXTERN,            // extern
    TOK_FALSE,             // false
    TOK_FROM,              // from
    TOK_FOR,               // for
    TOK_FOREACH,           // foreach
    TOK_FN,                // fn
    TOK_IF,                // if
    TOK_IMPORT,            // import
    TOK_IN,                // in
    TOK_IS,                // is
    TOK_INTERFACE,         // interface

    TOK_I8,                // i8
    TOK_I16,               // i16
    TOK_I32,               // i32
    TOK_I64,               // i64
    TOK_U8,                // u8
    TOK_U16,               // u16
    TOK_U32,               // u32
    TOK_U64,               // u64
    TOK_F32,               // f32
    TOK_F64,               // f64
    TOK_BOOLEAN,           // bool
    TOK_VOID,              // void
    TOK_STRING,            // string
    TOK_CHAR,              // char
    TOK_PTR,               // ptr
    TOK_VEC,               // vec
    TOK_PROCESS,           // process
    TOK_PROCESS_LINK,      // ::
    TOK_EMIT,              // emit
    TOK_SPAWN,             // spawn,
    TOK_NULLABLE,          // ?

    TOK_LET,               // let
    TOK_NEW,               // new
    TOK_NULL,              // null
    TOK_UNSAFE,            // unsafe
    TOK_RETURN,            // return
    //TOK_SIZEOF,          // sizeof
    //TOK_SUPER,           // super
    TOK_SELF,              // self
    //TOK_STATIC,          // static
    TOK_STRUCT,            // struct
    TOK_MATCH,             // match
    TOK_TRUE,              // true
    TOK_TYPE,              // type
    TOK_WHILE,             // while
    TOK_SEMICOLON,         // ;
    TOK_COLON,             // :
    TOK_COMMA,             // ,
    TOK_DOT,               // .
    TOK_DOTDOTDOT,         // ...
    TOK_LPAREN,            // (
    TOK_RPAREN,            // )
    TOK_LBRACKET,          // [
    TOK_RBRACKET,          // ]
    TOK_LBRACE,            // {
    TOK_RBRACE,            // }
    TOK_NOT,               // !
    TOK_EQUAL,             // =
    TOK_LOGICAL_OR,        // ||
    TOK_LOGICAL_AND,       // &&
    TOK_NOT_EQUAL,         // !=
    TOK_CASE_EXPR,         // =>
    TOK_DENULL,            // !!
    TOK_LESS,              // <
    TOK_LESS_EQUAL,        // <=
    TOK_GREATER,           // >
    TOK_GREATER_EQUAL,     // >=
    TOK_PLUS,              // +
    TOK_PLUS_EQUAL,        // +=
    TOK_MINUS,             // -
    TOK_MINUS_EQUAL,       // -=
    TOK_STAR,              // *
    TOK_STAR_EQUAL,        // *=
    TOK_DIV,               // /
    TOK_DIV_EQUAL,         // /=

    TOK_PERCENT,           // %
    TOK_BITWISE_NOT,       // ~
    TOK_BITWISE_OR,        // |
    TOK_BITWISE_XOR,       // ^
    TOK_BITWISE_AND,       // &
    TOK_LEFT_SHIFT,        // <<
    TOK_RIGHT_SHIFT,       // >>
    TOK_EQUAL_EQUAL,       // ==
    TOK_INCREMENT,         // ++
    TOK_DECREMENT,         // --
    TOK_FN_RETURN_TYPE,    // ->

    // Literals.
    TOK_WILDCARD,          // _
    TOK_IDENTIFIER,        // variable or function name
    TOK_STRING_VAL,        // string literal  (double quotes)
    TOK_CHAR_VAL,          // single character (single quote)
    TOK_INT,
    TOK_BINARY_INT,        // 0b[01]+
    TOK_OCT_INT,           // 0o[0-7]+
    TOK_HEX_INT,           // 0x[0-9A-F]+
    TOK_FLOAT,             //
    TOK_DOUBLE,
    TOK_EOF,
} TokenType;

const char* token_type_to_string(TokenType type);


#endif //TYPE_C_TOKENS_H
