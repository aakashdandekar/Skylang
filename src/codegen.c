#define _GNU_SOURCE
#include "../include/skylang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} Buffer;

static void buf_init(Buffer* b) {
    b->capacity = 1024;
    b->length = 0;
    b->data = (char*)malloc(b->capacity);
    b->data[0] = '\0';
}

static void buf_append(Buffer* b, const char* str, size_t len) {
    if (b->length + len + 1 >= b->capacity) {
        while (b->length + len + 1 >= b->capacity) {
            b->capacity *= 2;
        }
        b->data = (char*)realloc(b->data, b->capacity);
    }
    memcpy(b->data + b->length, str, len);
    b->length += len;
    b->data[b->length] = '\0';
}

static void buf_puts(Buffer* b, const char* str) {
    buf_append(b, str, strlen(str));
}

static void buf_printf(Buffer* b, const char* fmt, ...) {
    char temp[1024];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (n >= 0 && (size_t)n < sizeof(temp)) {
        buf_append(b, temp, (size_t)n);
    } else {
        va_start(args, fmt);
        char* dyn = NULL;
        int dn = vasprintf(&dyn, fmt, args);
        va_end(args);
        if (dn >= 0 && dyn) {
            buf_append(b, dyn, (size_t)dn);
            free(dyn);
        }
    }
}

static int temp_var_counter = 0;
static int loop_counter = 0;
static const char* codegen_source_filename = NULL;
static const char* codegen_current_fn = "<main>";

static const char* declared_vars[2048];
static size_t declared_vars_count = 0;

static void scope_clear(void) {
    declared_vars_count = 0;
}

static bool scope_has(const char* name) {
    for (size_t i = 0; i < declared_vars_count; ++i) {
        if (strcmp(declared_vars[i], name) == 0) return true;
    }
    return false;
}

static void scope_add(const char* name) {
    if (!scope_has(name) && declared_vars_count < 2048) {
        declared_vars[declared_vars_count++] = name;
    }
}

static void emit_expr(Buffer* b, AstNode* node, const char* current_class);
static void emit_statement(Buffer* b, AstNode* stmt, const char* current_class, int indent);

static void emit_indent(Buffer* b, int indent) {
    for (int i = 0; i < indent; ++i) {
        buf_puts(b, "    ");
    }
}

static void emit_expr(Buffer* b, AstNode* node, const char* current_class) {
    if (!node) {
        buf_puts(b, "val_nil()");
        return;
    }

    switch (node->type) {
        case AST_LITERAL: {
            switch (node->as.literal.lit_type) {
                case TOK_INT_LIT:
                    buf_printf(b, "val_int(%lldLL)", (long long)node->as.literal.as.i_val);
                    break;
                case TOK_DOUBLE_LIT:
                    buf_printf(b, "val_double(%g)", node->as.literal.as.d_val);
                    break;
                case TOK_STRING_LIT: {
                    buf_puts(b, "val_string(\"");
                    for (const char* p = node->as.literal.as.s_val; *p; ++p) {
                        if (*p == '"') buf_puts(b, "\\\"");
                        else if (*p == '\\') buf_puts(b, "\\\\");
                        else if (*p == '\n') buf_puts(b, "\\n");
                        else if (*p == '\t') buf_puts(b, "\\t");
                        else buf_append(b, p, 1);
                    }
                    buf_puts(b, "\")");
                    break;
                }
                case TOK_CHAR_LIT:
                    buf_printf(b, "val_char('%c')", node->as.literal.as.c_val);
                    break;
                case TOK_KW_TRUE:
                    buf_puts(b, "val_bool(true)");
                    break;
                case TOK_KW_FALSE:
                    buf_puts(b, "val_bool(false)");
                    break;
                case TOK_KW_NIL:
                    buf_puts(b, "val_nil()");
                    break;
                default:
                    buf_puts(b, "val_nil()");
                    break;
            }
            break;
        }
        case AST_VARIABLE: {
            if (strcmp(node->as.variable.name, "this") == 0 && current_class) {
                buf_puts(b, "sky_this");
            } else {
                buf_printf(b, "sky_var_%s", node->as.variable.name);
            }
            break;
        }
        case AST_THIS_VAR: {
            buf_printf(b, "val_get_prop(sky_this, \"%s\")", node->as.variable.name);
            break;
        }
        case AST_BINARY: {
            TokenType op = node->as.binary.op;
            switch (op) {
                case TOK_PLUS:
                    buf_puts(b, "val_add(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_MINUS:
                    buf_puts(b, "val_sub(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_STAR:
                    buf_puts(b, "val_mul(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_SLASH:
                    buf_puts(b, "val_div(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_FLOORDIV:
                    buf_puts(b, "val_floordiv(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_CARET:
                    buf_puts(b, "val_pow(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_PERCENT:
                    buf_puts(b, "val_mod(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_EQ:
                    buf_puts(b, "val_eq(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_NEQ:
                    buf_puts(b, "val_neq(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_LT:
                    buf_puts(b, "val_lt(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_LTE:
                    buf_puts(b, "val_lte(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_GT:
                    buf_puts(b, "val_gt(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_GTE:
                    buf_puts(b, "val_gte(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, ")");
                    break;
                case TOK_AND:
                    buf_puts(b, "val_bool(val_is_truthy(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ") && val_is_truthy(");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, "))");
                    break;
                case TOK_OR:
                    buf_puts(b, "val_bool(val_is_truthy(");
                    emit_expr(b, node->as.binary.left, current_class);
                    buf_puts(b, ") || val_is_truthy(");
                    emit_expr(b, node->as.binary.right, current_class);
                    buf_puts(b, "))");
                    break;
                default:
                    buf_puts(b, "val_nil()");
                    break;
            }
            break;
        }
        case AST_UNARY: {
            if (node->as.unary.op == TOK_MINUS) {
                buf_puts(b, "val_neg(");
                emit_expr(b, node->as.unary.operand, current_class);
                buf_puts(b, ")");
            } else if (node->as.unary.op == TOK_BANG) {
                buf_puts(b, "val_not(");
                emit_expr(b, node->as.unary.operand, current_class);
                buf_puts(b, ")");
            }
            break;
        }
        case AST_CALL: {
            int t = temp_var_counter++;
            size_t argc = node->as.call.args.count;
            buf_printf(b, "({ Value _callee_%d = ", t);
            emit_expr(b, node->as.call.callee, current_class);
            buf_printf(b, "; Value _args_%d[%zu]; const char* _kwnames_%d[%zu]; ",
                       t, argc > 0 ? argc : 1, t, argc > 0 ? argc : 1);
            for (size_t i = 0; i < argc; ++i) {
                AstNode* arg = node->as.call.args.items[i];
                if (arg->type == AST_NAMED_ARG) {
                    buf_printf(b, "_args_%d[%zu] = ", t, i);
                    emit_expr(b, arg->as.named_arg.value, current_class);
                    buf_printf(b, "; _kwnames_%d[%zu] = \"%s\"; ", t, i, arg->as.named_arg.name);
                } else {
                    buf_printf(b, "_args_%d[%zu] = ", t, i);
                    emit_expr(b, arg, current_class);
                    buf_printf(b, "; _kwnames_%d[%zu] = NULL; ", t, i);
                }
            }
            buf_printf(b, "val_call_kw(_callee_%d, %zu, _args_%d, _kwnames_%d); })", t, argc, t, t);
            break;
        }
        case AST_METHOD_CALL: {
            int t = temp_var_counter++;
            size_t argc = node->as.method_call.args.count;
            buf_printf(b, "({ Value _tgt_%d = ", t);
            emit_expr(b, node->as.method_call.target, current_class);
            buf_printf(b, "; Value _margs_%d[%zu]; const char* _mkwnames_%d[%zu]; ",
                       t, argc > 0 ? argc : 1, t, argc > 0 ? argc : 1);
            for (size_t i = 0; i < argc; ++i) {
                AstNode* arg = node->as.method_call.args.items[i];
                if (arg->type == AST_NAMED_ARG) {
                    buf_printf(b, "_margs_%d[%zu] = ", t, i);
                    emit_expr(b, arg->as.named_arg.value, current_class);
                    buf_printf(b, "; _mkwnames_%d[%zu] = \"%s\"; ", t, i, arg->as.named_arg.name);
                } else {
                    buf_printf(b, "_margs_%d[%zu] = ", t, i);
                    emit_expr(b, arg, current_class);
                    buf_printf(b, "; _mkwnames_%d[%zu] = NULL; ", t, i);
                }
            }
            buf_printf(b, "val_call_method_kw(_tgt_%d, \"%s\", %zu, _margs_%d, _mkwnames_%d); })",
                       t, node->as.method_call.method_name, argc, t, t);
            break;
        }
        case AST_PROP_GET: {
            buf_puts(b, "val_get_prop(");
            emit_expr(b, node->as.prop.target, current_class);
            buf_printf(b, ", \"%s\")", node->as.prop.prop_name);
            break;
        }
        case AST_INDEX_GET: {
            buf_puts(b, "val_get_index(");
            emit_expr(b, node->as.index.target, current_class);
            buf_puts(b, ", ");
            emit_expr(b, node->as.index.index, current_class);
            buf_puts(b, ")");
            break;
        }
        case AST_SLICE: {
            buf_puts(b, "val_slice(");
            emit_expr(b, node->as.slice.target, current_class);
            buf_puts(b, ", ");
            if (node->as.slice.start) emit_expr(b, node->as.slice.start, current_class);
            else buf_puts(b, "val_nil()");
            buf_puts(b, ", ");
            if (node->as.slice.end) emit_expr(b, node->as.slice.end, current_class);
            else buf_puts(b, "val_nil()");
            buf_puts(b, ")");
            break;
        }
        case AST_ARRAY_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.collection.items.count;
            buf_printf(b, "({ Value _arr_%d = val_array(%zu, TYPE_INT); ObjArray* _oa_%d = as_array(_arr_%d); ", t, count, t, t);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "_oa_%d->items[%zu] = ", t, i);
                emit_expr(b, node->as.collection.items.items[i], current_class);
                buf_puts(b, "; ");
            }
            buf_printf(b, "_arr_%d; })", t);
            break;
        }
        case AST_LIST_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.collection.items.count;
            buf_printf(b, "({ Value _lst_%d = val_list(); ObjList* _ol_%d = as_list(_lst_%d); ", t, t, t);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "list_push(_ol_%d, ", t);
                emit_expr(b, node->as.collection.items.items[i], current_class);
                buf_puts(b, "); ");
            }
            buf_printf(b, "_lst_%d; })", t);
            break;
        }
        case AST_TUPLE_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.collection.items.count;
            buf_printf(b, "({ Value _titems_%d[%zu]; ", t, count > 0 ? count : 1);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "_titems_%d[%zu] = ", t, i);
                emit_expr(b, node->as.collection.items.items[i], current_class);
                buf_puts(b, "; ");
            }
            buf_printf(b, "val_tuple(%zu, _titems_%d); })", count, t);
            break;
        }
        case AST_DICT_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.dict_lit.keys.count;
            buf_printf(b, "({ Value _d_%d = val_dict(); ObjDict* _od_%d = as_dict(_d_%d); ", t, t, t);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "dict_set(_od_%d, ", t);
                emit_expr(b, node->as.dict_lit.keys.items[i], current_class);
                buf_puts(b, ", ");
                emit_expr(b, node->as.dict_lit.values.items[i], current_class);
                buf_puts(b, "); ");
            }
            buf_printf(b, "_d_%d; })", t);
            break;
        }
        case AST_SET_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.collection.items.count;
            buf_printf(b, "({ Value _s_%d = val_set(); ObjSet* _os_%d = as_set(_s_%d); ", t, t, t);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "set_add(_os_%d, ", t);
                emit_expr(b, node->as.collection.items.items[i], current_class);
                buf_puts(b, "); ");
            }
            buf_printf(b, "_s_%d; })", t);
            break;
        }
        case AST_STMT_ASSIGN: {
            AstNode* tgt = node->as.assign.target;
            TokenType op = node->as.assign.op;
            if (tgt->type == AST_VARIABLE) {
                if (op == TOK_ASSIGN || op == TOK_WALRUS) {
                    buf_printf(b, "(sky_var_%s = ", tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_PLUS_ASSIGN) {
                    buf_printf(b, "(sky_var_%s = val_add(sky_var_%s, ", tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_MINUS_ASSIGN) {
                    buf_printf(b, "(sky_var_%s = val_sub(sky_var_%s, ", tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_STAR_ASSIGN) {
                    buf_printf(b, "(sky_var_%s = val_mul(sky_var_%s, ", tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_SLASH_ASSIGN) {
                    buf_printf(b, "(sky_var_%s = val_div(sky_var_%s, ", tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                }
            } else if (tgt->type == AST_THIS_VAR) {
                if (op == TOK_ASSIGN || op == TOK_WALRUS) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", ", tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_PLUS_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_add(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_MINUS_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_sub(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_STAR_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_mul(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                } else if (op == TOK_SLASH_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_div(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, node->as.assign.value, current_class);
                    buf_puts(b, "))");
                }
            }
            break;
        }
        case AST_INDEX_SET: {
            buf_puts(b, "val_set_index(");
            emit_expr(b, node->as.index.target, current_class);
            buf_puts(b, ", ");
            emit_expr(b, node->as.index.index, current_class);
            buf_puts(b, ", ");
            emit_expr(b, node->as.index.value, current_class);
            buf_puts(b, ")");
            break;
        }
        case AST_PROP_SET: {
            buf_puts(b, "val_set_prop(");
            emit_expr(b, node->as.prop.target, current_class);
            buf_printf(b, ", \"%s\", ", node->as.prop.prop_name);
            emit_expr(b, node->as.prop.value, current_class);
            buf_puts(b, ")");
            break;
        }
        default:
            buf_puts(b, "val_nil()");
            break;
    }
}

static void emit_statement(Buffer* b, AstNode* stmt, const char* current_class, int indent) {
    if (!stmt) return;

    if (codegen_source_filename && stmt->line > 0) {
        buf_printf(b, "#line %d \"%s\"\n", stmt->line, codegen_source_filename);
    }
    emit_indent(b, indent);
    if (codegen_source_filename && stmt->line > 0) {
        buf_printf(b, "sky_set_loc(\"%s\", %d, \"%s\"); ",
                   codegen_source_filename, stmt->line, codegen_current_fn ? codegen_current_fn : "<main>");
    }

    switch (stmt->type) {
        case AST_STMT_EXPR: {
            if (stmt->as.expr_stmt.expr->type == AST_CALL &&
                stmt->as.expr_stmt.expr->as.call.callee->type == AST_VARIABLE &&
                strcmp(stmt->as.expr_stmt.expr->as.call.callee->as.variable.name, "takes") == 0) {
                buf_puts(b, "\n");
                return;
            }
            emit_expr(b, stmt->as.expr_stmt.expr, current_class);
            buf_puts(b, ";\n");
            break;
        }
        case AST_STMT_VAR_DECL: {
            const char* name = stmt->as.var_decl.name;
            TokenType tt = stmt->as.var_decl.type_tok;
            if (scope_has(name)) {
                buf_printf(b, "sky_var_%s = ", name);
            } else {
                scope_add(name);
                buf_printf(b, "Value sky_var_%s = ", name);
            }
            if (stmt->as.var_decl.init_expr) {
                emit_expr(b, stmt->as.var_decl.init_expr, current_class);
            } else if (stmt->as.var_decl.size_expr) {

                buf_puts(b, "val_array((size_t)(");
                emit_expr(b, stmt->as.var_decl.size_expr, current_class);
                buf_puts(b, ").as.i, ");
                switch (tt) {
                    case TOK_KW_I: buf_puts(b, "TYPE_INT"); break;
                    case TOK_KW_D: buf_puts(b, "TYPE_DOUBLE"); break;
                    case TOK_KW_B: buf_puts(b, "TYPE_BOOL"); break;
                    case TOK_KW_C: buf_puts(b, "TYPE_CHAR"); break;
                    case TOK_KW_S: buf_puts(b, "TYPE_STRING"); break;
                    default: buf_puts(b, "TYPE_NIL"); break;
                }
                buf_puts(b, ")");
            } else {
                switch (tt) {
                    case TOK_KW_I: buf_puts(b, "val_int(0)"); break;
                    case TOK_KW_D: buf_puts(b, "val_double(0.0)"); break;
                    case TOK_KW_B: buf_puts(b, "val_bool(false)"); break;
                    case TOK_KW_C: buf_puts(b, "val_char('a')"); break;
                    case TOK_KW_S: buf_puts(b, "val_string(\"\")"); break;
                    case TOK_KW_L: buf_puts(b, "val_list()"); break;
                    case TOK_KW_T: buf_puts(b, "val_tuple(0, NULL)"); break;
                    case TOK_KW_SL: buf_puts(b, "val_sorted_list()"); break;
                    case TOK_KW_DICT: buf_puts(b, "val_dict()"); break;
                    case TOK_KW_SET: buf_puts(b, "val_set()"); break;
                    default: buf_puts(b, "val_nil()"); break;
                }
            }
            buf_puts(b, ";\n");
            break;
        }
        case AST_STMT_ASSIGN: {
            AstNode* tgt = stmt->as.assign.target;
            TokenType op = stmt->as.assign.op;
            if (tgt->type == AST_VARIABLE) {
                if (!scope_has(tgt->as.variable.name)) {
                    scope_add(tgt->as.variable.name);
                    buf_printf(b, "Value sky_var_%s = ", tgt->as.variable.name);
                } else {
                    buf_printf(b, "sky_var_%s = ", tgt->as.variable.name);
                }
                if (op == TOK_ASSIGN || op == TOK_WALRUS) {
                    emit_expr(b, stmt->as.assign.value, current_class);
                } else if (op == TOK_PLUS_ASSIGN) {
                    buf_printf(b, "val_add(sky_var_%s, ", tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_MINUS_ASSIGN) {
                    buf_printf(b, "val_sub(sky_var_%s, ", tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_STAR_ASSIGN) {
                    buf_printf(b, "val_mul(sky_var_%s, ", tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_SLASH_ASSIGN) {
                    buf_printf(b, "val_div(sky_var_%s, ", tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                }
                buf_puts(b, ";\n");
            } else if (tgt->type == AST_THIS_VAR) {
                if (op == TOK_ASSIGN || op == TOK_WALRUS) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", ", tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ");\n");
                } else if (op == TOK_PLUS_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_add(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, "));\n");
                } else if (op == TOK_MINUS_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_sub(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, "));\n");
                } else if (op == TOK_STAR_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_mul(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, "));\n");
                } else if (op == TOK_SLASH_ASSIGN) {
                    buf_printf(b, "val_set_prop(sky_this, \"%s\", val_div(val_get_prop(sky_this, \"%s\"), ",
                               tgt->as.variable.name, tgt->as.variable.name);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, "));\n");
                }
            }
            break;
        }
        case AST_STMT_MULTI_VAR_DECL:
        case AST_STMT_MULTI_ASSIGN: {
            int t = temp_var_counter++;
            buf_printf(b, "Value _mres_%d = ", t);
            emit_expr(b, stmt->as.multi_assign.expr, current_class);
            buf_puts(b, ";\n");
            for (size_t i = 0; i < stmt->as.multi_assign.count; ++i) {
                const char* vname = stmt->as.multi_assign.names[i];
                emit_indent(b, indent);
                if (strcmp(vname, "_") == 0) {
                    buf_printf(b, "(void)val_unpack(_mres_%d, %zu);\n", t, i);
                } else if (scope_has(vname)) {
                    buf_printf(b, "sky_var_%s = val_unpack(_mres_%d, %zu);\n", vname, t, i);
                } else {
                    scope_add(vname);
                    buf_printf(b, "Value sky_var_%s = val_unpack(_mres_%d, %zu);\n", vname, t, i);
                }
            }
            break;
        }
        case AST_INDEX_SET: {
            buf_puts(b, "val_set_index(");
            emit_expr(b, stmt->as.index.target, current_class);
            buf_puts(b, ", ");
            emit_expr(b, stmt->as.index.index, current_class);
            buf_puts(b, ", ");
            emit_expr(b, stmt->as.index.value, current_class);
            buf_puts(b, ");\n");
            break;
        }
        case AST_PROP_SET: {
            buf_puts(b, "val_set_prop(");
            emit_expr(b, stmt->as.prop.target, current_class);
            buf_printf(b, ", \"%s\", ", stmt->as.prop.prop_name);
            emit_expr(b, stmt->as.prop.value, current_class);
            buf_puts(b, ");\n");
            break;
        }
        case AST_STMT_RETURN: {
            int t = temp_var_counter++;
            buf_printf(b, "Value _ret_%d = ", t);
            if (stmt->as.ret.expr) {
                emit_expr(b, stmt->as.ret.expr, current_class);
            } else {
                buf_puts(b, "val_nil()");
            }
            buf_puts(b, "; ");
            buf_puts(b, "sky_pop_frame(); ");
            buf_printf(b, "return _ret_%d;\n", t);
            break;
        }
        case AST_STMT_IF: {
            buf_puts(b, "if (val_is_truthy(");
            emit_expr(b, stmt->as.if_stmt.cond, current_class);
            buf_puts(b, ")) {\n");
            for (size_t i = 0; i < stmt->as.if_stmt.then_branch->as.block.statements.count; ++i) {
                emit_statement(b, stmt->as.if_stmt.then_branch->as.block.statements.items[i], current_class, indent + 1);
            }
            if (stmt->as.if_stmt.else_branch) {
                emit_indent(b, indent);
                buf_puts(b, "} else {\n");
                if (stmt->as.if_stmt.else_branch->type == AST_STMT_BLOCK) {
                    for (size_t i = 0; i < stmt->as.if_stmt.else_branch->as.block.statements.count; ++i) {
                        emit_statement(b, stmt->as.if_stmt.else_branch->as.block.statements.items[i], current_class, indent + 1);
                    }
                } else {
                    emit_statement(b, stmt->as.if_stmt.else_branch, current_class, indent + 1);
                }
            }
            emit_indent(b, indent);
            buf_puts(b, "}\n");
            break;
        }
        case AST_STMT_WHILE: {
            buf_puts(b, "while (val_is_truthy(");
            emit_expr(b, stmt->as.while_stmt.cond, current_class);
            buf_puts(b, ")) {\n");
            for (size_t i = 0; i < stmt->as.while_stmt.body->as.block.statements.count; ++i) {
                emit_statement(b, stmt->as.while_stmt.body->as.block.statements.items[i], current_class, indent + 1);
            }
            emit_indent(b, indent);
            buf_puts(b, "}\n");
            break;
        }
        case AST_STMT_FOR_IN: {
            int l = loop_counter++;
            buf_printf(b, "{\n");
            emit_indent(b, indent + 1);
            buf_printf(b, "Value _iter_col_%d = ", l);
            emit_expr(b, stmt->as.for_in.iter_expr, current_class);
            buf_puts(b, ";\n");

            emit_indent(b, indent + 1);
            buf_printf(b, "int64_t _iter_len_%d = val_get_prop(_iter_col_%d, \"size\").as.i;\n", l, l);

            emit_indent(b, indent + 1);
            buf_printf(b, "for (int64_t _iter_i_%d = 0; _iter_i_%d < _iter_len_%d; ++_iter_i_%d) {\n", l, l, l, l);

            emit_indent(b, indent + 2);
            buf_printf(b, "Value sky_var_%s = val_get_index(_iter_col_%d, val_int(_iter_i_%d));\n", stmt->as.for_in.var_name, l, l);

            for (size_t i = 0; i < stmt->as.for_in.body->as.block.statements.count; ++i) {
                emit_statement(b, stmt->as.for_in.body->as.block.statements.items[i], current_class, indent + 2);
            }

            emit_indent(b, indent + 1);
            buf_puts(b, "}\n");
            emit_indent(b, indent);
            buf_puts(b, "}\n");
            break;
        }
        case AST_STMT_BREAK:
            buf_puts(b, "break;\n");
            break;
        case AST_STMT_CONTINUE:
            buf_puts(b, "continue;\n");
            break;
        case AST_STMT_BLOCK: {
            buf_puts(b, "{\n");
            for (size_t i = 0; i < stmt->as.block.statements.count; ++i) {
                emit_statement(b, stmt->as.block.statements.items[i], current_class, indent + 1);
            }
            emit_indent(b, indent);
            buf_puts(b, "}\n");
            break;
        }
        default:
            break;
    }
}

static void emit_function(Buffer* b, AstNode* fn, const char* class_prefix) {
    scope_clear();
    char func_c_name[256];
    char func_display_name[256];
    if (class_prefix) {
        snprintf(func_c_name, sizeof(func_c_name), "sky_method_%s_%s", class_prefix, fn->as.fn_decl.name);
        snprintf(func_display_name, sizeof(func_display_name), "%s.%s", class_prefix, fn->as.fn_decl.name);
    } else {
        snprintf(func_c_name, sizeof(func_c_name), "sky_fn_%s", fn->as.fn_decl.name);
        snprintf(func_display_name, sizeof(func_display_name), "%s", fn->as.fn_decl.name);
    }

    const char* prev_fn = codegen_current_fn;
    codegen_current_fn = func_display_name;

    buf_printf(b, "static Value %s(int argc, Value* argv) {\n", func_c_name);
    if (codegen_source_filename && fn->line > 0) {
        buf_printf(b, "    sky_push_frame(\"%s\", %d, \"%s\");\n", codegen_source_filename, fn->line, func_display_name);
    }
    int arg_offset = 0;
    if (class_prefix) {
        buf_puts(b, "    Value sky_this = (argc > 0) ? argv[0] : val_nil();\n");
        arg_offset = 1;
    }

    buf_puts(b, "    Value sky_var_args = val_list();\n");
    buf_printf(b, "    for (int _ai = %d; _ai < argc; ++_ai) list_push(as_list(sky_var_args), argv[_ai]);\n", arg_offset);
    scope_add("args");

    size_t pcount = fn->as.fn_decl.param_count;
    for (size_t i = 0; i < pcount; ++i) {
        scope_add(fn->as.fn_decl.params[i]);
        buf_printf(b, "    Value sky_var_%s = (argc > %d) ? argv[%d] : val_nil();\n",
                   fn->as.fn_decl.params[i], (int)i + arg_offset, (int)i + arg_offset);
    }

    AstNode* body = fn->as.fn_decl.body;
    for (size_t i = 0; i < body->as.block.statements.count; ++i) {
        emit_statement(b, body->as.block.statements.items[i], class_prefix, 1);
    }

    buf_puts(b, "    sky_pop_frame();\n");
    buf_puts(b, "    return val_nil();\n");
    buf_puts(b, "}\n\n");
    codegen_current_fn = prev_fn;
}

char* codegen_emit_c(AstNode* root, const char* filename) {
    codegen_source_filename = filename;
    codegen_current_fn = "<main>";
    Buffer b;
    buf_init(&b);

    buf_puts(&b, "#include \"skylang_rt.h\"\n");
    buf_puts(&b, "#include \"sky_stdlib.h\"\n");
    buf_puts(&b, "#include \"sky_python.h\"\n");
    buf_puts(&b, "#include \"sky_js.h\"\n");
    buf_puts(&b, "#include \"sky_cpp.h\"\n");
    buf_puts(&b, "#include \"sky_java.h\"\n");
    buf_puts(&b, "#include \"sky_go.h\"\n");
    buf_puts(&b, "#include \"sky_rust.h\"\n");
    buf_puts(&b, "#include <stdio.h>\n");
    buf_puts(&b, "#include <stdlib.h>\n\n");

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_CIMPORT) {
            if (stmt->as.cimport.headers && stmt->as.cimport.count > 0) {
                for (size_t h = 0; h < stmt->as.cimport.count; ++h) {
                    buf_printf(&b, "#include \"%s\"\n", stmt->as.cimport.headers[h]);
                }
            } else if (stmt->as.cimport.header) {
                buf_printf(&b, "#include \"%s\"\n", stmt->as.cimport.header);
            }
        }
    }
    buf_puts(&b, "\n");

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_EXTERN_DECL) {
            const char* ename = stmt->as.extern_decl.name;
            size_t epcount = stmt->as.extern_decl.param_count;
            buf_printf(&b, "static Value sky_extern_wrap_%s(int argc, Value* argv) {\n", ename);

            for (size_t p = 0; p < epcount; ++p) {
                buf_printf(&b, "    double _a%zu = (argc > %zu) ? ((argv[%zu].type == VAL_INT) ? (double)argv[%zu].as.i : argv[%zu].as.d) : 0.0;\n",
                           p, p, p, p, p);
            }
            buf_printf(&b, "    double _result = %s(", ename);
            for (size_t p = 0; p < epcount; ++p) {
                buf_printf(&b, "_a%zu%s", p, (p + 1 < epcount) ? ", " : "");
            }
            buf_puts(&b, ");\n");
            buf_puts(&b, "    return val_double(_result);\n");
            buf_puts(&b, "}\n\n");
        }
    }

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_FN_DECL) {
            buf_printf(&b, "static Value sky_fn_%s(int argc, Value* argv);\n", stmt->as.fn_decl.name);
        } else if (stmt->type == AST_STMT_CLASS_DECL) {
            for (size_t m = 0; m < stmt->as.class_decl.methods.count; ++m) {
                AstNode* mfn = stmt->as.class_decl.methods.items[m];
                buf_printf(&b, "static Value sky_method_%s_%s(int argc, Value* argv);\n",
                           stmt->as.class_decl.name, mfn->as.fn_decl.name);
            }
        }
    }
    buf_puts(&b, "\n");

    buf_puts(&b, "static Value sky_var_print;\n");
    buf_puts(&b, "static Value sky_var_println;\n");
    buf_puts(&b, "static Value sky_var_eval;\n");
    buf_puts(&b, "static Value sky_var_range;\n");
    buf_puts(&b, "static Value sky_var_len;\n");
    buf_puts(&b, "static Value sky_var_type;\n");
    buf_puts(&b, "static Value sky_var_takes;\n");
    buf_puts(&b, "static Value sky_var_free;\n");
    buf_puts(&b, "static Value sky_var_gc;\n");
    buf_puts(&b, "static Value sky_var_panic;\n");
    buf_puts(&b, "static Value sky_var_error;\n");
    buf_puts(&b, "static Value sky_var_split;\n");
    buf_puts(&b, "static Value sky_var_join;\n");
    buf_puts(&b, "static Value sky_var_upper;\n");
    buf_puts(&b, "static Value sky_var_lower;\n");
    buf_puts(&b, "static Value sky_var_trim;\n");
    buf_puts(&b, "static Value sky_var_trimleft;\n");
    buf_puts(&b, "static Value sky_var_trimright;\n");
    buf_puts(&b, "static Value sky_var_contains;\n");
    buf_puts(&b, "static Value sky_var_startswith;\n");
    buf_puts(&b, "static Value sky_var_endswith;\n");
    buf_puts(&b, "static Value sky_var_replace;\n");
    buf_puts(&b, "static Value sky_var_find;\n");
    buf_puts(&b, "static Value sky_var_count;\n");
    buf_puts(&b, "static Value sky_var_reverse;\n");
    buf_puts(&b, "static Value sky_var_chars;\n");
    buf_puts(&b, "static Value sky_var_bytes;\n");
    buf_puts(&b, "static Value sky_var_math;\n");
    buf_puts(&b, "static Value sky_var_io;\n");
    buf_puts(&b, "static Value sky_var_fmt;\n");
    buf_puts(&b, "static Value sky_var_python;\n");
    buf_puts(&b, "static Value sky_var_js;\n");
    buf_puts(&b, "static Value sky_var_cpp;\n");
    buf_puts(&b, "static Value sky_var_java;\n");
    buf_puts(&b, "static Value sky_var_go;\n");
    buf_puts(&b, "static Value sky_var_golang;\n");
    buf_puts(&b, "static Value sky_var_rust;\n");

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_EXTERN_DECL) {
            buf_printf(&b, "static Value sky_var_%s;\n", stmt->as.extern_decl.name);
        }
    }

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_FN_DECL) {
            buf_printf(&b, "static Value sky_var_%s;\n", stmt->as.fn_decl.name);
        } else if (stmt->type == AST_STMT_CLASS_DECL) {
            buf_printf(&b, "static Value sky_var_%s;\n", stmt->as.class_decl.name);
        }
    }
    buf_puts(&b, "\n");

    buf_puts(&b, "static Value sky_builtin_print(int argc, Value* argv) {\n");
    buf_puts(&b, "    for (int i = 0; i < argc; ++i) {\n");
    buf_puts(&b, "        val_print(argv[i]);\n");
    buf_puts(&b, "        if (i + 1 < argc) putchar(' ');\n");
    buf_puts(&b, "    }\n");
    buf_puts(&b, "    putchar('\\n');\n");
    buf_puts(&b, "    return val_nil();\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_range(int argc, Value* argv) {\n");
    buf_puts(&b, "    int64_t start = 0, end = 0, step = 1;\n");
    buf_puts(&b, "    if (argc == 1) { end = argv[0].as.i; }\n");
    buf_puts(&b, "    else if (argc >= 2) { start = argv[0].as.i; end = argv[1].as.i; }\n");
    buf_puts(&b, "    if (argc >= 3) { step = argv[2].as.i; }\n");
    buf_puts(&b, "    Value res = val_list();\n");
    buf_puts(&b, "    ObjList* l = as_list(res);\n");
    buf_puts(&b, "    if (step > 0) {\n");
    buf_puts(&b, "        for (int64_t i = start; i < end; i += step) list_push(l, val_int(i));\n");
    buf_puts(&b, "    } else if (step < 0) {\n");
    buf_puts(&b, "        for (int64_t i = start; i > end; i += step) list_push(l, val_int(i));\n");
    buf_puts(&b, "    }\n");
    buf_puts(&b, "    return res;\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_len(int argc, Value* argv) {\n");
    buf_puts(&b, "    if (argc >= 1) return val_get_prop(argv[0], \"size\");\n");
    buf_puts(&b, "    return val_int(0);\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_type(int argc, Value* argv) {\n");
    buf_puts(&b, "    if (argc >= 1) return val_get_type(argv[0]);\n");
    buf_puts(&b, "    return val_type(TYPE_NIL);\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_takes(int argc, Value* argv) {\n");
    buf_puts(&b, "    (void)argc; (void)argv;\n");
    buf_puts(&b, "    return val_nil();\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_gc(int argc, Value* argv) {\n");
    buf_puts(&b, "    (void)argc; (void)argv;\n");
    buf_puts(&b, "    GC_gcollect();\n");
    buf_puts(&b, "    return val_nil();\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_free(int argc, Value* argv) {\n");
    buf_puts(&b, "    if (argc >= 1 && argv[0].type == VAL_OBJ) {\n");
    buf_puts(&b, "        GC_FREE(argv[0].as.obj);\n");
    buf_puts(&b, "    }\n");
    buf_puts(&b, "    return val_nil();\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_panic(int argc, Value* argv) {\n");
    buf_puts(&b, "    fprintf(stderr, \"panic: \");\n");
    buf_puts(&b, "    for (int i = 0; i < argc; ++i) { val_print(argv[i]); if (i + 1 < argc) fputc(' ', stderr); }\n");
    buf_puts(&b, "    fputc('\\n', stderr);\n");
    buf_puts(&b, "    exit(1);\n");
    buf_puts(&b, "}\n\n");

    buf_puts(&b, "static Value sky_builtin_error(int argc, Value* argv) {\n");
    buf_puts(&b, "    if (argc >= 1) {\n");
    buf_puts(&b, "        char* s = val_to_string(argv[0]);\n");
    buf_puts(&b, "        Value err = val_error(s);\n");
    buf_puts(&b, "        free(s);\n");
    buf_puts(&b, "        return err;\n");
    buf_puts(&b, "    }\n");
    buf_puts(&b, "    return val_error(\"error\");\n");
    buf_puts(&b, "}\n\n");

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_FN_DECL) {
            emit_function(&b, stmt, NULL);
        } else if (stmt->type == AST_STMT_CLASS_DECL) {
            for (size_t m = 0; m < stmt->as.class_decl.methods.count; ++m) {
                emit_function(&b, stmt->as.class_decl.methods.items[m], stmt->as.class_decl.name);
            }
        }
    }

    buf_puts(&b, "int main(int argc, char** argv) {\n");
    buf_puts(&b, "    (void)argc; (void)argv;\n");
    buf_puts(&b, "    sky_init_runtime();\n");
    if (filename) {
        buf_printf(&b, "    sky_set_loc(\"%s\", 1, \"<main>\");\n\n", filename);
    } else {
        buf_puts(&b, "    sky_set_loc(\"<main>\", 1, \"<main>\");\n\n");
    }

    buf_puts(&b, "    sky_var_print = val_function(\"print\", sky_builtin_print, -1);\n");
    buf_puts(&b, "    sky_var_println = val_function(\"println\", sky_builtin_print, -1);\n");
    buf_puts(&b, "    sky_var_eval = val_function(\"eval\", sky_builtin_eval, 1);\n");
    buf_puts(&b, "    sky_var_range = val_function(\"range\", sky_builtin_range, -1);\n");
    buf_puts(&b, "    sky_var_len = val_function(\"len\", sky_builtin_len, 1);\n");
    buf_puts(&b, "    sky_var_type = val_function(\"type\", sky_builtin_type, 1);\n");
    buf_puts(&b, "    sky_var_takes = val_function(\"takes\", sky_builtin_takes, -1);\n");
    buf_puts(&b, "    sky_var_gc = val_function(\"gc\", sky_builtin_gc, 0);\n");
    buf_puts(&b, "    sky_var_free = val_function(\"free\", sky_builtin_free, 1);\n");
    buf_puts(&b, "    sky_var_panic = val_function(\"panic\", sky_builtin_panic, -1);\n");
    buf_puts(&b, "    sky_var_error = val_function(\"error\", sky_builtin_error, 1);\n");
    buf_puts(&b, "    sky_var_split = val_function(\"split\", sky_builtin_split, -1);\n");
    buf_puts(&b, "    sky_var_join = val_function(\"join\", sky_builtin_join, -1);\n");
    buf_puts(&b, "    sky_var_upper = val_function(\"upper\", sky_builtin_upper, 1);\n");
    buf_puts(&b, "    sky_var_lower = val_function(\"lower\", sky_builtin_lower, 1);\n");
    buf_puts(&b, "    sky_var_trim = val_function(\"trim\", sky_builtin_trim, 1);\n");
    buf_puts(&b, "    sky_var_trimleft = val_function(\"trimleft\", sky_builtin_trimleft, 1);\n");
    buf_puts(&b, "    sky_var_trimright = val_function(\"trimright\", sky_builtin_trimright, 1);\n");
    buf_puts(&b, "    sky_var_contains = val_function(\"contains\", sky_builtin_contains, 2);\n");
    buf_puts(&b, "    sky_var_startswith = val_function(\"startswith\", sky_builtin_startswith, 2);\n");
    buf_puts(&b, "    sky_var_endswith = val_function(\"endswith\", sky_builtin_endswith, 2);\n");
    buf_puts(&b, "    sky_var_replace = val_function(\"replace\", sky_builtin_replace, 3);\n");
    buf_puts(&b, "    sky_var_find = val_function(\"find\", sky_builtin_find, 2);\n");
    buf_puts(&b, "    sky_var_count = val_function(\"count\", sky_builtin_count, 2);\n");
    buf_puts(&b, "    sky_var_reverse = val_function(\"reverse\", sky_builtin_reverse, 1);\n");
    buf_puts(&b, "    sky_var_chars = val_function(\"chars\", sky_builtin_chars, 1);\n");
    buf_puts(&b, "    sky_var_bytes = val_function(\"bytes\", sky_builtin_bytes, 1);\n\n");

    buf_puts(&b, "    sky_stdlib_init_all();\n");
    buf_puts(&b, "    sky_var_math = sky_mod_math;\n");
    buf_puts(&b, "    sky_var_io = sky_mod_io;\n");
    buf_puts(&b, "    sky_var_fmt = sky_mod_fmt;\n");
    buf_puts(&b, "    sky_var_python = sky_python_init();\n");
    buf_puts(&b, "    sky_var_js = sky_js_init();\n");
    buf_puts(&b, "    sky_var_cpp = sky_cpp_init();\n");
    buf_puts(&b, "    sky_var_java = sky_java_init();\n");
    buf_puts(&b, "    sky_var_go = sky_go_init();\n");
    buf_puts(&b, "    sky_var_golang = sky_go_init();\n");
    buf_puts(&b, "    sky_var_rust = sky_rust_init();\n\n");

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_EXTERN_DECL) {
            buf_printf(&b, "    sky_var_%s = val_function(\"%s\", sky_extern_wrap_%s, %zu);\n",
                       stmt->as.extern_decl.name, stmt->as.extern_decl.name,
                       stmt->as.extern_decl.name, stmt->as.extern_decl.param_count);
        }
    }

    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_FN_DECL) {
            if (stmt->as.fn_decl.param_count > 0) {
                buf_printf(&b, "    static const char* _params_%s[] = {", stmt->as.fn_decl.name);
                for (size_t p = 0; p < stmt->as.fn_decl.param_count; ++p) {
                    buf_printf(&b, "\"%s\"%s", stmt->as.fn_decl.params[p], (p + 1 < stmt->as.fn_decl.param_count) ? ", " : "");
                }
                buf_printf(&b, "};\n");
                buf_printf(&b, "    sky_var_%s = val_function_with_params(\"%s\", sky_fn_%s, %zu, %zu, _params_%s);\n",
                           stmt->as.fn_decl.name, stmt->as.fn_decl.name, stmt->as.fn_decl.name,
                           stmt->as.fn_decl.param_count, stmt->as.fn_decl.param_count, stmt->as.fn_decl.name);
            } else {
                buf_printf(&b, "    sky_var_%s = val_function(\"%s\", sky_fn_%s, 0);\n",
                           stmt->as.fn_decl.name, stmt->as.fn_decl.name, stmt->as.fn_decl.name);
            }
        } else if (stmt->type == AST_STMT_CLASS_DECL) {
            const char* cname = stmt->as.class_decl.name;
            buf_printf(&b, "    sky_var_%s = val_class(\"%s\");\n", cname, cname);
            buf_printf(&b, "    ObjClass* _cls_%s = as_class(sky_var_%s);\n", cname, cname);
            for (size_t m = 0; m < stmt->as.class_decl.methods.count; ++m) {
                AstNode* mfn = stmt->as.class_decl.methods.items[m];
                buf_printf(&b, "    {\n");
                buf_printf(&b, "        MethodEntry* me = (MethodEntry*)GC_MALLOC(sizeof(MethodEntry));\n");
                buf_printf(&b, "        me->name = \"%s\";\n", mfn->as.fn_decl.name);
                if (mfn->as.fn_decl.param_count > 0) {
                    buf_printf(&b, "        static const char* _mparams_%s_%s[] = {", cname, mfn->as.fn_decl.name);
                    for (size_t p = 0; p < mfn->as.fn_decl.param_count; ++p) {
                        buf_printf(&b, "\"%s\"%s", mfn->as.fn_decl.params[p], (p + 1 < mfn->as.fn_decl.param_count) ? ", " : "");
                    }
                    buf_printf(&b, "};\n");
                    buf_printf(&b, "        me->fn = val_function_with_params(\"%s\", sky_method_%s_%s, %zu, %zu, _mparams_%s_%s);\n",
                               mfn->as.fn_decl.name, cname, mfn->as.fn_decl.name, mfn->as.fn_decl.param_count + 1,
                               mfn->as.fn_decl.param_count, cname, mfn->as.fn_decl.name);
                } else {
                    buf_printf(&b, "        me->fn = val_function(\"%s\", sky_method_%s_%s, 0);\n",
                               mfn->as.fn_decl.name, cname, mfn->as.fn_decl.name);
                }
                buf_printf(&b, "        me->next = _cls_%s->methods;\n", cname);
                buf_printf(&b, "        _cls_%s->methods = me;\n", cname);
                buf_printf(&b, "    }\n");
            }
        }
    }
    buf_puts(&b, "\n");

    scope_clear();
    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type != AST_STMT_FN_DECL && stmt->type != AST_STMT_CLASS_DECL &&
            stmt->type != AST_STMT_EXTERN_DECL && stmt->type != AST_STMT_CIMPORT &&
            stmt->type != AST_STMT_IMPORT) {
            emit_statement(&b, stmt, NULL, 1);
        }
    }

    buf_puts(&b, "\n    return 0;\n");
    buf_puts(&b, "}\n");

    return b.data;
}

