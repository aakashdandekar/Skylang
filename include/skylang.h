#ifndef SKYLANG_H
#define SKYLANG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sky_platform.h"

/* Token types */
typedef enum {
    /* End of file / Error */
    TOK_EOF,
    TOK_ERROR,

    /* Literals */
    TOK_INT_LIT,
    TOK_DOUBLE_LIT,
    TOK_STRING_LIT,
    TOK_CHAR_LIT,
    TOK_FSTRING_LIT,  /* f"..." or f'...' */
    TOK_IDENT,

    /* Keywords - Scalar types */
    TOK_KW_I,     /* int */
    TOK_KW_D,     /* double */
    TOK_KW_B,     /* bool */
    TOK_KW_C,     /* char */
    TOK_KW_S,     /* string */

    /* Keywords - Collection types */
    TOK_KW_L,     /* list */
    TOK_KW_T,     /* tuple or .T */
    TOK_KW_SL,    /* sortedList */
    TOK_KW_DICT,  /* dict */
    TOK_KW_SET,   /* set */

    /* Other keywords */
    TOK_KW_F,
    TOK_KW_RETURN,
    TOK_KW_CLASS,
    TOK_KW_INIT,
    TOK_KW_THIS,
    TOK_KW_TAKES,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_ELIF,
    TOK_KW_WHILE,
    TOK_KW_FOR,
    TOK_KW_IN,
    TOK_KW_BREAK,
    TOK_KW_CONTINUE,
    TOK_KW_TRUE,
    TOK_KW_FALSE,
    TOK_KW_NIL,
    TOK_KW_NEW,
    TOK_KW_EXTERN,    /* extern */
    TOK_KW_CIMPORT,   /* cimport */
    TOK_KW_IMPORT,    /* import */
    TOK_KW_FROM,      /* from */
    TOK_KW_AS,        /* as */
    TOK_KW_ASYNC,     /* async */
    TOK_KW_AWAIT,     /* await */
    TOK_KW_SPAWN,     /* spawn */

    /* Operators */
    TOK_PLUS,         /* + */
    TOK_MINUS,        /* - */
    TOK_STAR,         /* * */
    TOK_SLASH,        /* / */
    TOK_FLOORDIV,     /* // */
    TOK_CARET,        /* ^ (power) */
    TOK_PERCENT,      /* % */

    TOK_ASSIGN,       /* = */
    TOK_WALRUS,       /* := */
    TOK_PLUS_ASSIGN,  /* += */
    TOK_MINUS_ASSIGN, /* -= */
    TOK_STAR_ASSIGN,  /* *= */
    TOK_SLASH_ASSIGN, /* /= */

    TOK_EQ,           /* == */
    TOK_NEQ,          /* != */
    TOK_LT,           /* < */
    TOK_LTE,          /* <= */
    TOK_GT,           /* > */
    TOK_GTE,          /* >= */

    TOK_BANG,         /* ! */
    TOK_AND,          /* && or and */
    TOK_OR,           /* || or or */

    /* Delimiters */
    TOK_LPAREN,       /* ( */
    TOK_RPAREN,       /* ) */
    TOK_LBRACE,       /* { */
    TOK_RBRACE,       /* } */
    TOK_LBRACKET,     /* [ */
    TOK_RBRACKET,     /* ] */
    TOK_DOT,          /* . */
    TOK_COMMA,        /* , */
    TOK_COLON,        /* : */
    TOK_SEMICOLON     /* ; */
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
    int col;
    union {
        int64_t i_val;
        double d_val;
        char c_val;
        char* s_val;
    } as;
} Token;

typedef struct {
    const char* source;
    const char* current;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
const char* token_type_name(TokenType type);

/* AST definitions */
typedef enum {
    /* Expressions */
    AST_LITERAL,
    AST_VARIABLE,
    AST_THIS_VAR,
    AST_BINARY,
    AST_UNARY,
    AST_CALL,
    AST_METHOD_CALL,
    AST_PROP_GET,
    AST_PROP_SET,
    AST_INDEX_GET,
    AST_INDEX_SET,
    AST_SLICE,
    AST_LIST_LIT,
    AST_ARRAY_LIT,
    AST_TUPLE_LIT,
    AST_DICT_LIT,
    AST_SET_LIT,
    AST_SORTED_LIST_LIT,
    AST_FSTRING,
    AST_EXPR_AWAIT,
    AST_EXPR_SPAWN,

    /* Statements */
    AST_STMT_EXPR,
    AST_STMT_VAR_DECL,     /* I x, x L, x I 5, etc */
    AST_STMT_ASSIGN,       /* x = y or x := y */
    AST_STMT_MULTI_VAR_DECL, /* a, b := expr */
    AST_STMT_MULTI_ASSIGN,   /* a, b = expr */
    AST_STMT_RETURN,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_FOR_IN,
    AST_STMT_BREAK,
    AST_STMT_CONTINUE,
    AST_STMT_BLOCK,
    AST_STMT_FN_DECL,
    AST_STMT_CLASS_DECL,
    AST_NAMED_ARG,
    AST_STMT_EXTERN_DECL,  /* extern f name(params) */
    AST_STMT_CIMPORT,      /* cimport "header.h" */
    AST_STMT_IMPORT,       /* import module [as alias], ... */
    AST_STMT_FROM_IMPORT,  /* from module import symbol [as alias], ... or * */
    AST_PROGRAM
} AstNodeType;

typedef struct {
    char* path;     /* e.g. "math_utils" or "sub/helper" or "python" */
    char* alias;    /* e.g. "mu" or "sh" or "math_utils" */
} ImportItem;

typedef struct {
    char* symbol;   /* e.g. "add" or "MyClass" */
    char* alias;    /* e.g. "my_add" (or same as symbol) */
} FromImportItem;

typedef struct AstNode AstNode;

typedef struct {
    AstNode** items;
    size_t count;
    size_t capacity;
} AstNodeList;

struct AstNode {
    AstNodeType type;
    int line;

    union {
        /* AST_LITERAL */
        struct {
            TokenType lit_type;
            union {
                int64_t i_val;
                double d_val;
                bool b_val;
                char c_val;
                char* s_val;
            } as;
        } literal;

        /* AST_VARIABLE, AST_THIS_VAR */
        struct {
            char* name;
        } variable;

        /* AST_BINARY */
        struct {
            TokenType op;
            AstNode* left;
            AstNode* right;
        } binary;

        /* AST_UNARY */
        struct {
            TokenType op;
            AstNode* operand;
        } unary;

        /* AST_CALL */
        struct {
            AstNode* callee;
            AstNodeList args;
        } call;

        /* AST_NAMED_ARG */
        struct {
            char* name;
            AstNode* value;
        } named_arg;

        /* AST_METHOD_CALL */
        struct {
            AstNode* target;
            char* method_name;
            AstNodeList args;
        } method_call;

        /* AST_PROP_GET, AST_PROP_SET */
        struct {
            AstNode* target;
            char* prop_name;
            AstNode* value; /* for PROP_SET */
        } prop;

        /* AST_INDEX_GET, AST_INDEX_SET */
        struct {
            AstNode* target;
            AstNode* index;
            AstNode* value; /* for INDEX_SET */
        } index;

        /* AST_SLICE */
        struct {
            AstNode* target;
            AstNode* start;
            AstNode* end;
        } slice;

        /* AST_LIST_LIT, AST_ARRAY_LIT, AST_TUPLE_LIT, AST_SET_LIT, AST_SORTED_LIST_LIT */
        struct {
            TokenType elem_type; /* for array */
            AstNodeList items;
        } collection;

        /* AST_DICT_LIT */
        struct {
            AstNodeList keys;
            AstNodeList values;
        } dict_lit;

        /* AST_FSTRING */
        struct {
            AstNodeList parts;
        } fstring;

        /* AST_STMT_VAR_DECL */
        struct {
            char* name;
            TokenType type_tok; /* TOK_KW_I, TOK_KW_D, etc */
            AstNode* size_expr; /* for array */
            AstNode* init_expr; /* for init */
            bool is_walrus;
        } var_decl;

        /* AST_STMT_ASSIGN */
        struct {
            AstNode* target;
            TokenType op;
            AstNode* value;
        } assign;

        /* AST_STMT_MULTI_VAR_DECL, AST_STMT_MULTI_ASSIGN */
        struct {
            char** names;
            size_t count;
            AstNode* expr;
        } multi_assign;

        /* AST_STMT_RETURN */
        struct {
            AstNode* expr;
        } ret;

        /* AST_STMT_IF */
        struct {
            AstNode* cond;
            AstNode* then_branch;
            AstNode* else_branch;
        } if_stmt;

        /* AST_STMT_WHILE */
        struct {
            AstNode* cond;
            AstNode* body;
        } while_stmt;

        /* AST_STMT_FOR_IN */
        struct {
            char* var_name;
            AstNode* iter_expr;
            AstNode* body;
        } for_in;

        /* AST_STMT_BLOCK, AST_PROGRAM */
        struct {
            AstNodeList statements;
        } block;

        /* AST_STMT_FN_DECL */
        struct {
            char* name;
            char** params;
            size_t param_count;
            AstNode* body;
            bool is_async;
        } fn_decl;

        /* AST_STMT_CLASS_DECL */
        struct {
            char* name;
            char** fields;
            size_t field_count;
            AstNodeList methods; /* list of AST_STMT_FN_DECL */
        } class_decl;

        /* AST_STMT_EXPR */
        struct {
            AstNode* expr;
        } expr_stmt;

        /* AST_STMT_EXTERN_DECL */
        struct {
            char* name;
            char** params;
            size_t param_count;
        } extern_decl;

        /* AST_STMT_CIMPORT */
        struct {
            char* header;
            char** headers;
            size_t count;
        } cimport;

        /* AST_STMT_IMPORT */
        struct {
            ImportItem* items;
            size_t count;
            char* module_name;
            char** module_names;
        } import_stmt;

        /* AST_STMT_FROM_IMPORT */
        struct {
            char* module_path;
            FromImportItem* items;
            size_t count;
            bool is_wildcard;
        } from_import;
    } as;
};

/* Parser */
typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
    int angle_bracket_depth;
    const char* filename;
} Parser;

void parser_init(Parser* parser, const char* source, const char* filename);
AstNode* parse_program(Parser* parser);

/* Codegen */
char* codegen_emit_c(AstNode* root, const char* filename);
int codegen_compile_and_run(const char* source, bool keep_binary, const char* output_bin, const char* emit_c_path);

#endif /* SKYLANG_H */
