#define _GNU_SOURCE
#include "../include/skylang.h"
#include "../include/sky_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

/* Module Management System */
typedef struct {
    int mod_id;
    char* mod_path;
    char* mod_name;
    char* filepath;
    AstNode* ast;
} LoadedModule;

typedef struct {
    LoadedModule* items;
    size_t count;
    size_t capacity;
} ModuleRegistry;

static ModuleRegistry registry = { NULL, 0, 0 };

static void registry_init(void) {
    registry.items = NULL;
    registry.count = 0;
    registry.capacity = 0;
}

static bool file_readable(const char* path) {
    return sky_access(path, R_OK) == 0;
}

static char* read_file_contents(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

static char* sanitize_identifier(const char* str) {
    if (!str) return strdup("mod");
    char* s = strdup(str);
    for (char* p = s; *p; ++p) {
        if (*p == '/' || *p == '.' || *p == '\\' || *p == '-') {
            *p = '_';
        }
    }
    return s;
}

static bool is_foreign_module(const char* name) {
    if (!name) return false;
    return (strcmp(name, "python") == 0 ||
            strcmp(name, "js") == 0 ||
            strcmp(name, "cpp") == 0 ||
            strcmp(name, "java") == 0 ||
            strcmp(name, "go") == 0 ||
            strcmp(name, "golang") == 0 ||
            strcmp(name, "rust") == 0);
}

static bool is_stdlib_module(const char* name) {
    if (!name) return false;
    return (strcmp(name, "math") == 0 ||
            strcmp(name, "io") == 0 ||
            strcmp(name, "fmt") == 0 ||
            strcmp(name, "async") == 0 ||
            strcmp(name, "str") == 0);
}

static char* resolve_module_file(const char* base_dir, const char* mod_path) {
    char candidate[1024];

    if (file_readable(mod_path)) {
        return strdup(mod_path);
    }

    snprintf(candidate, sizeof(candidate), "%s.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    if (base_dir && strlen(base_dir) > 0) {
        snprintf(candidate, sizeof(candidate), "%s/%s.sky", base_dir, mod_path);
        if (file_readable(candidate)) return strdup(candidate);

        snprintf(candidate, sizeof(candidate), "%s/%s/init.sky", base_dir, mod_path);
        if (file_readable(candidate)) return strdup(candidate);

        snprintf(candidate, sizeof(candidate), "%s/%s/mod.sky", base_dir, mod_path);
        if (file_readable(candidate)) return strdup(candidate);

        snprintf(candidate, sizeof(candidate), "%s/%s", base_dir, mod_path);
        if (file_readable(candidate)) return strdup(candidate);
    }

    snprintf(candidate, sizeof(candidate), "./%s.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "./%s/init.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "./%s/mod.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "./%s", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "examples/%s.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "examples/%s/init.sky", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    snprintf(candidate, sizeof(candidate), "examples/%s", mod_path);
    if (file_readable(candidate)) return strdup(candidate);

    return NULL;
}

static LoadedModule* find_loaded_module(ModuleRegistry* reg, const char* filepath, const char* mod_path) {
    for (size_t i = 0; i < reg->count; ++i) {
        if (filepath && reg->items[i].filepath && strcmp(reg->items[i].filepath, filepath) == 0) {
            return &reg->items[i];
        }
        if (mod_path && reg->items[i].mod_path && strcmp(reg->items[i].mod_path, mod_path) == 0) {
            return &reg->items[i];
        }
    }
    return NULL;
}

static void load_modules_from_ast(ModuleRegistry* reg, AstNode* node, const char* current_file_path);

static LoadedModule* load_module_file(ModuleRegistry* reg, const char* mod_path, const char* current_file_path) {
    if (!mod_path) return NULL;
    if (is_foreign_module(mod_path) || is_stdlib_module(mod_path)) return NULL;

    char base_dir[1024] = {0};
    if (current_file_path) {
        sky_extract_dirname(current_file_path, base_dir, sizeof(base_dir));
    }

    char* resolved = resolve_module_file(base_dir[0] ? base_dir : NULL, mod_path);
    if (!resolved) {
        return NULL;
    }

    LoadedModule* existing = find_loaded_module(reg, resolved, mod_path);
    if (existing) {
        free(resolved);
        return existing;
    }

    char* source = read_file_contents(resolved);
    if (!source) {
        free(resolved);
        return NULL;
    }

    Parser parser;
    parser_init(&parser, source, resolved);
    AstNode* mast = parse_program(&parser);
    if (!mast || parser.had_error) {
        free(source);
        free(resolved);
        return NULL;
    }

    if (reg->count >= reg->capacity) {
        reg->capacity = reg->capacity < 8 ? 8 : reg->capacity * 2;
        reg->items = (LoadedModule*)realloc(reg->items, reg->capacity * sizeof(LoadedModule));
    }

    int id = (int)(reg->count + 1);
    char* mod_name = sanitize_identifier(mod_path);

    LoadedModule* mod = &reg->items[reg->count++];
    mod->mod_id = id;
    mod->mod_path = strdup(mod_path);
    mod->mod_name = mod_name;
    mod->filepath = resolved;
    mod->ast = mast;

    load_modules_from_ast(reg, mast, resolved);
    return mod;
}

static void load_modules_from_ast(ModuleRegistry* reg, AstNode* node, const char* current_file_path) {
    if (!node) return;
    if (node->type == AST_PROGRAM || node->type == AST_STMT_BLOCK) {
        for (size_t i = 0; i < node->as.block.statements.count; ++i) {
            load_modules_from_ast(reg, node->as.block.statements.items[i], current_file_path);
        }
    } else if (node->type == AST_STMT_IMPORT) {
        for (size_t i = 0; i < node->as.import_stmt.count; ++i) {
            const char* mpath = node->as.import_stmt.items ? node->as.import_stmt.items[i].path : node->as.import_stmt.module_names[i];
            load_module_file(reg, mpath, current_file_path);
        }
    } else if (node->type == AST_STMT_FROM_IMPORT) {
        load_module_file(reg, node->as.from_import.module_path, current_file_path);
    }
}

static void extract_foreign_symbols_from_ast(AstNode* node, const char* filepath, ForeignSymbolTable* table) {
    if (!node) return;
    if (node->type == AST_PROGRAM || node->type == AST_STMT_BLOCK) {
        for (size_t i = 0; i < node->as.block.statements.count; ++i) {
            extract_foreign_symbols_from_ast(node->as.block.statements.items[i], filepath, table);
        }
    } else if (node->type == AST_STMT_EXPR) {
        AstNode* expr = node->as.expr_stmt.expr;
        if (expr && expr->type == AST_METHOD_CALL && strcmp(expr->as.method_call.method_name, "exec") == 0) {
            AstNode* target = expr->as.method_call.target;
            AstNode* code_arg = expr->as.method_call.args.count > 0 ? expr->as.method_call.args.items[0] : NULL;
            const char* code_str = (code_arg && code_arg->type == AST_LITERAL && code_arg->as.literal.lit_type == TOK_STRING_LIT)
                                   ? code_arg->as.literal.as.s_val : NULL;
            if (target && target->type == AST_VARIABLE && code_str) {
                if (strcmp(target->as.variable.name, "js") == 0) {
                    sky_extract_js_declarations(code_str, filepath, node->line, table);
                } else if (strcmp(target->as.variable.name, "python") == 0) {
                    sky_extract_py_declarations(code_str, filepath, node->line, table);
                }
            }
        }
    }
}

static void validate_foreign_scopes(ModuleRegistry* reg, AstNode* root, const char* root_file) {
    ForeignSymbolTable root_syms;
    foreign_symtable_init(&root_syms);
    extract_foreign_symbols_from_ast(root, root_file ? root_file : "<main>", &root_syms);

    for (size_t m = 0; m < reg->count; ++m) {
        LoadedModule* mod = &reg->items[m];
        ForeignSymbolTable mod_syms;
        foreign_symtable_init(&mod_syms);
        extract_foreign_symbols_from_ast(mod->ast, mod->filepath, &mod_syms);

        for (size_t ms = 0; ms < mod_syms.count; ++ms) {
            ForeignSymbol* msym = &mod_syms.items[ms];
            if (strcmp(msym->lang, "js") == 0 && msym->kind == FOREIGN_DECL_VAR) {
                for (size_t rs = 0; rs < root_syms.count; ++rs) {
                    ForeignSymbol* rsym = &root_syms.items[rs];
                    if (strcmp(rsym->name, msym->name) == 0 && strcmp(rsym->lang, "python") == 0) {
                        fprintf(stderr, "Traceback (most recent call last):\n");
                        fprintf(stderr, "  File \"%s\", line %d, in <main>\n", rsym->file, rsym->line);
                        fprintf(stderr, "ScopeError: variable '%s' in Python scope conflicts with global JavaScript 'var' variable '%s' imported from '%s'\n",
                                rsym->name, msym->name, msym->file);
                        exit(1);
                    }
                }
            } else if (strcmp(msym->lang, "python") == 0) {
                for (size_t rs = 0; rs < root_syms.count; ++rs) {
                    ForeignSymbol* rsym = &root_syms.items[rs];
                    if (strcmp(rsym->name, msym->name) == 0 && strcmp(rsym->lang, "js") == 0 && rsym->kind == FOREIGN_DECL_VAR) {
                        fprintf(stderr, "Traceback (most recent call last):\n");
                        fprintf(stderr, "  File \"%s\", line %d, in <main>\n", rsym->file, rsym->line);
                        fprintf(stderr, "ScopeError: global JavaScript 'var' variable '%s' conflicts with Python variable '%s' imported from '%s'\n",
                                rsym->name, msym->name, msym->file);
                        exit(1);
                    }
                }
            }
        }
        foreign_symtable_free(&mod_syms);
    }
    foreign_symtable_free(&root_syms);
}

static void add_global_id(const char* name, const char** vars, size_t* count, size_t max) {
    if (!name || strcmp(name, "_") == 0 || strcmp(name, "this") == 0) return;
    if (strcmp(name, "js") == 0 || strcmp(name, "python") == 0 ||
        strcmp(name, "cpp") == 0 || strcmp(name, "java") == 0 ||
        strcmp(name, "go") == 0 || strcmp(name, "golang") == 0 ||
        strcmp(name, "rust") == 0 || strcmp(name, "async") == 0) return;
    for (size_t i = 0; i < *count; ++i) {
        if (strcmp(vars[i], name) == 0) return;
    }
    if (*count < max) {
        vars[(*count)++] = strdup(name);
    }
}

static void collect_global_identifiers(AstNode* node, const char** vars, size_t* count, size_t max) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
        case AST_STMT_BLOCK: {
            for (size_t i = 0; i < node->as.block.statements.count; ++i) {
                collect_global_identifiers(node->as.block.statements.items[i], vars, count, max);
            }
            break;
        }
        case AST_STMT_EXPR: {
            AstNode* expr = node->as.expr_stmt.expr;
            if (expr && expr->type == AST_METHOD_CALL && strcmp(expr->as.method_call.method_name, "exec") == 0) {
                AstNode* target = expr->as.method_call.target;
                AstNode* code_arg = expr->as.method_call.args.count > 0 ? expr->as.method_call.args.items[0] : NULL;
                const char* code_str = (code_arg && code_arg->type == AST_LITERAL && code_arg->as.literal.lit_type == TOK_STRING_LIT)
                                       ? code_arg->as.literal.as.s_val : NULL;
                if (target && target->type == AST_VARIABLE && code_str) {
                    ForeignSymbolTable syms;
                    foreign_symtable_init(&syms);
                    if (strcmp(target->as.variable.name, "js") == 0) {
                        sky_extract_js_declarations(code_str, "<js>", 1, &syms);
                    } else if (strcmp(target->as.variable.name, "python") == 0) {
                        sky_extract_py_declarations(code_str, "<python>", 1, &syms);
                    }
                    for (size_t si = 0; si < syms.count; ++si) {
                        add_global_id(syms.items[si].name, vars, count, max);
                    }
                    foreign_symtable_free(&syms);
                }
            }
            break;
        }
        case AST_STMT_VAR_DECL:
            add_global_id(node->as.var_decl.name, vars, count, max);
            break;
        case AST_STMT_ASSIGN:
            if (node->as.assign.target && node->as.assign.target->type == AST_VARIABLE) {
                add_global_id(node->as.assign.target->as.variable.name, vars, count, max);
            }
            break;
        case AST_STMT_MULTI_VAR_DECL:
        case AST_STMT_MULTI_ASSIGN:
            for (size_t i = 0; i < node->as.multi_assign.count; ++i) {
                add_global_id(node->as.multi_assign.names[i], vars, count, max);
            }
            break;
        case AST_STMT_FN_DECL:
            add_global_id(node->as.fn_decl.name, vars, count, max);
            break;
        case AST_STMT_CLASS_DECL:
            add_global_id(node->as.class_decl.name, vars, count, max);
            break;
        case AST_STMT_EXTERN_DECL:
            add_global_id(node->as.extern_decl.name, vars, count, max);
            break;
        case AST_STMT_IMPORT:
            for (size_t i = 0; i < node->as.import_stmt.count; ++i) {
                if (node->as.import_stmt.items) {
                    add_global_id(node->as.import_stmt.items[i].alias, vars, count, max);
                } else if (node->as.import_stmt.module_names) {
                    add_global_id(node->as.import_stmt.module_names[i], vars, count, max);
                }
            }
            break;
        case AST_STMT_FROM_IMPORT:
            if (node->as.from_import.is_wildcard) {
                LoadedModule* mod = find_loaded_module(&registry, NULL, node->as.from_import.module_path);
                if (mod) {
                    for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
                        AstNode* mstmt = mod->ast->as.block.statements.items[s];
                        if (mstmt->type == AST_STMT_FN_DECL) {
                            add_global_id(mstmt->as.fn_decl.name, vars, count, max);
                        } else if (mstmt->type == AST_STMT_CLASS_DECL) {
                            add_global_id(mstmt->as.class_decl.name, vars, count, max);
                        } else if (mstmt->type == AST_STMT_VAR_DECL) {
                            add_global_id(mstmt->as.var_decl.name, vars, count, max);
                        } else if (mstmt->type == AST_STMT_ASSIGN && mstmt->as.assign.target->type == AST_VARIABLE) {
                            add_global_id(mstmt->as.assign.target->as.variable.name, vars, count, max);
                        }
                    }
                }
            } else if (node->as.from_import.items) {
                for (size_t i = 0; i < node->as.from_import.count; ++i) {
                    add_global_id(node->as.from_import.items[i].alias ? node->as.from_import.items[i].alias : node->as.from_import.items[i].symbol, vars, count, max);
                }
            }
            break;
        default:
            break;
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
            } else if (node->as.unary.op == TOK_KW_AWAIT) {
                buf_puts(b, "val_future_await(");
                emit_expr(b, node->as.unary.operand, current_class);
                buf_puts(b, ")");
            } else if (node->as.unary.op == TOK_KW_SPAWN) {
                AstNode* operand = node->as.unary.operand;
                if (operand->type == AST_CALL) {
                    int t = temp_var_counter++;
                    size_t argc = operand->as.call.args.count;
                    buf_printf(b, "({ Value _callee_%d = ", t);
                    emit_expr(b, operand->as.call.callee, current_class);
                    buf_printf(b, "; Value _sargs_%d[%zu]; ", t, argc + 1);
                    buf_printf(b, "_sargs_%d[0] = _callee_%d; ", t, t);
                    for (size_t i = 0; i < argc; ++i) {
                        AstNode* arg = operand->as.call.args.items[i];
                        buf_printf(b, "_sargs_%d[%zu] = ", t, i + 1);
                        if (arg->type == AST_NAMED_ARG) {
                            emit_expr(b, arg->as.named_arg.value, current_class);
                        } else {
                            emit_expr(b, arg, current_class);
                        }
                        buf_puts(b, "; ");
                    }
                    buf_printf(b, "sky_async_spawn(%zu, _sargs_%d); })", argc + 1, t);
                } else {
                    int t = temp_var_counter++;
                    buf_printf(b, "({ Value _sargs_%d[1]; _sargs_%d[0] = ", t, t);
                    emit_expr(b, operand, current_class);
                    buf_printf(b, "; sky_async_spawn(1, _sargs_%d); })", t);
                }
            }
            break;
        }
        case AST_EXPR_AWAIT: {
            buf_puts(b, "val_future_await(");
            emit_expr(b, node->as.unary.operand, current_class);
            buf_puts(b, ")");
            break;
        }
        case AST_EXPR_SPAWN: {
            AstNode* operand = node->as.unary.operand;
            if (operand->type == AST_CALL) {
                int t = temp_var_counter++;
                size_t argc = operand->as.call.args.count;
                buf_printf(b, "({ Value _callee_%d = ", t);
                emit_expr(b, operand->as.call.callee, current_class);
                buf_printf(b, "; Value _sargs_%d[%zu]; ", t, argc + 1);
                buf_printf(b, "_sargs_%d[0] = _callee_%d; ", t, t);
                for (size_t i = 0; i < argc; ++i) {
                    AstNode* arg = operand->as.call.args.items[i];
                    buf_printf(b, "_sargs_%d[%zu] = ", t, i + 1);
                    if (arg->type == AST_NAMED_ARG) {
                        emit_expr(b, arg->as.named_arg.value, current_class);
                    } else {
                        emit_expr(b, arg, current_class);
                    }
                    buf_puts(b, "; ");
                }
                buf_printf(b, "sky_async_spawn(%zu, _sargs_%d); })", argc + 1, t);
            } else {
                int t = temp_var_counter++;
                buf_printf(b, "({ Value _sargs_%d[1]; _sargs_%d[0] = ", t, t);
                emit_expr(b, operand, current_class);
                buf_printf(b, "; sky_async_spawn(1, _sargs_%d); })", t);
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
        case AST_SORTED_LIST_LIT: {
            int t = temp_var_counter++;
            size_t count = node->as.collection.items.count;
            buf_printf(b, "({ Value _sl_%d = val_sorted_list(); ObjSortedList* _osl_%d = as_sorted_list(_sl_%d); ", t, t, t);
            for (size_t i = 0; i < count; ++i) {
                buf_printf(b, "sorted_list_push(_osl_%d, ", t);
                emit_expr(b, node->as.collection.items.items[i], current_class);
                buf_puts(b, "); ");
            }
            buf_printf(b, "_sl_%d; })", t);
            break;
        }
        case AST_FSTRING: {
            size_t count = node->as.fstring.parts.count;
            if (count == 0) {
                buf_puts(b, "val_string(\"\")");
            } else {
                for (size_t i = 0; i < count; ++i) {
                    buf_puts(b, "val_add(");
                }
                buf_puts(b, "val_string(\"\")");
                for (size_t i = 0; i < count; ++i) {
                    buf_puts(b, ", ");
                    emit_expr(b, node->as.fstring.parts.items[i], current_class);
                    buf_puts(b, ")");
                }
            }
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
        emit_indent(b, indent);
        buf_printf(b, "sky_set_loc(\"%s\", %d, \"%s\");\n", codegen_source_filename, stmt->line, codegen_current_fn);
    }

    emit_indent(b, indent);

    switch (stmt->type) {
        case AST_STMT_EXPR: {
            AstNode* expr = stmt->as.expr_stmt.expr;
            if (expr && expr->type == AST_METHOD_CALL && strcmp(expr->as.method_call.method_name, "exec") == 0) {
                AstNode* target = expr->as.method_call.target;
                AstNode* code_arg = expr->as.method_call.args.count > 0 ? expr->as.method_call.args.items[0] : NULL;
                const char* code_str = (code_arg && code_arg->type == AST_LITERAL && code_arg->as.literal.lit_type == TOK_STRING_LIT)
                                       ? code_arg->as.literal.as.s_val : NULL;

                int t = temp_var_counter++;
                buf_printf(b, "Value _exec_res_%d = ", t);
                emit_expr(b, expr, current_class);
                buf_puts(b, ";\n");

                if (target && target->type == AST_VARIABLE && code_str) {
                    ForeignSymbolTable syms;
                    foreign_symtable_init(&syms);
                    if (strcmp(target->as.variable.name, "js") == 0) {
                        sky_extract_js_declarations(code_str, codegen_source_filename, stmt->line, &syms);
                    } else if (strcmp(target->as.variable.name, "python") == 0) {
                        sky_extract_py_declarations(code_str, codegen_source_filename, stmt->line, &syms);
                    }

                    if (syms.count > 0) {
                        emit_indent(b, indent);
                        buf_printf(b, "if (_exec_res_%d.type == VAL_OBJ && _exec_res_%d.as.obj->type == OBJ_DICT) {\n", t, t);
                        emit_indent(b, indent + 1);
                        buf_printf(b, "ObjDict* _ed_%d = (ObjDict*)_exec_res_%d.as.obj;\n", t, t);
                        for (size_t si = 0; si < syms.count; ++si) {
                            const char* sname = syms.items[si].name;
                            emit_indent(b, indent + 1);
                            buf_printf(b, "{\n");
                            emit_indent(b, indent + 2);
                            buf_printf(b, "Value _v = dict_get(_ed_%d, val_string(\"%s\"));\n", t, sname);
                            emit_indent(b, indent + 2);
                            buf_printf(b, "if (_v.type != VAL_NIL) { sky_var_%s = _v; }\n", sname);
                            emit_indent(b, indent + 1);
                            buf_printf(b, "}\n");
                        }
                        emit_indent(b, indent);
                        buf_puts(b, "}\n");
                    }
                    foreign_symtable_free(&syms);
                }
            } else {
                emit_expr(b, stmt->as.expr_stmt.expr, current_class);
                buf_puts(b, ";\n");
            }
            break;
        }
        case AST_STMT_VAR_DECL: {
            const char* vname = stmt->as.var_decl.name;
            if (scope_has(vname)) {
                buf_printf(b, "sky_var_%s = ", vname);
            } else {
                scope_add(vname);
                buf_printf(b, "Value sky_var_%s = ", vname);
            }
            if (stmt->as.var_decl.init_expr) {
                emit_expr(b, stmt->as.var_decl.init_expr, current_class);
            } else {
                switch (stmt->as.var_decl.type_tok) {
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
                const char* vname = tgt->as.variable.name;
                if (!scope_has(vname)) {
                    scope_add(vname);
                    buf_printf(b, "Value sky_var_%s = ", vname);
                } else {
                    buf_printf(b, "sky_var_%s = ", vname);
                }

                if (op == TOK_ASSIGN || op == TOK_WALRUS) {
                    emit_expr(b, stmt->as.assign.value, current_class);
                } else if (op == TOK_PLUS_ASSIGN) {
                    buf_printf(b, "val_add(sky_var_%s, ", vname);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_MINUS_ASSIGN) {
                    buf_printf(b, "val_sub(sky_var_%s, ", vname);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_STAR_ASSIGN) {
                    buf_printf(b, "val_mul(sky_var_%s, ", vname);
                    emit_expr(b, stmt->as.assign.value, current_class);
                    buf_puts(b, ")");
                } else if (op == TOK_SLASH_ASSIGN) {
                    buf_printf(b, "val_div(sky_var_%s, ", vname);
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
        case AST_STMT_IMPORT: {
            for (size_t i = 0; i < stmt->as.import_stmt.count; ++i) {
                const char* mpath = stmt->as.import_stmt.items ? stmt->as.import_stmt.items[i].path : stmt->as.import_stmt.module_names[i];
                const char* alias = stmt->as.import_stmt.items ? stmt->as.import_stmt.items[i].alias : mpath;
                if (!alias) alias = mpath;

                if (strcmp(mpath, "python") == 0) {
                    buf_printf(b, "sky_var_%s = sky_python_init();\n", alias);
                } else if (strcmp(mpath, "js") == 0) {
                    buf_printf(b, "sky_var_%s = sky_js_init();\n", alias);
                } else if (strcmp(mpath, "cpp") == 0) {
                    buf_printf(b, "sky_var_%s = sky_cpp_init();\n", alias);
                } else if (strcmp(mpath, "java") == 0) {
                    buf_printf(b, "sky_var_%s = sky_java_init();\n", alias);
                } else if (strcmp(mpath, "go") == 0 || strcmp(mpath, "golang") == 0) {
                    buf_printf(b, "sky_var_%s = sky_go_init();\n", alias);
                } else if (strcmp(mpath, "rust") == 0) {
                    buf_printf(b, "sky_var_%s = sky_rust_init();\n", alias);
                } else if (strcmp(mpath, "math") == 0) {
                    buf_printf(b, "sky_var_%s = sky_mod_math;\n", alias);
                } else if (strcmp(mpath, "async") == 0) {
                    buf_printf(b, "sky_var_%s = sky_mod_async;\n", alias);
                } else {
                    LoadedModule* mod = find_loaded_module(&registry, NULL, mpath);
                    if (mod) {
                        buf_printf(b, "sky_var_%s = sky_m%d_init();\n", alias, mod->mod_id);
                    }
                }
            }
            break;
        }
        case AST_STMT_FROM_IMPORT: {
            const char* mpath = stmt->as.from_import.module_path;
            if (strcmp(mpath, "python") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_python_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "js") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_js_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "go") == 0 || strcmp(mpath, "golang") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_go_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "rust") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_rust_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "cpp") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_cpp_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "java") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_java_init();\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "math") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_mod_math;\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else if (strcmp(mpath, "async") == 0) {
                int t = temp_var_counter++;
                buf_printf(b, "Value _mmod_%d = sky_mod_async;\n", t);
                emit_indent(b, indent);
                buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                    FromImportItem* item = &stmt->as.from_import.items[i];
                    emit_indent(b, indent);
                    buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                               item->alias ? item->alias : item->symbol, t, item->symbol);
                }
            } else {
                LoadedModule* mod = find_loaded_module(&registry, NULL, mpath);
                if (mod) {
                    int t = temp_var_counter++;
                    buf_printf(b, "Value _mmod_%d = sky_m%d_init();\n", t, mod->mod_id);
                    emit_indent(b, indent);
                    buf_printf(b, "ObjDict* _mdict_%d = as_dict(_mmod_%d);\n", t, t);
                    if (stmt->as.from_import.is_wildcard) {
                        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
                            AstNode* mstmt = mod->ast->as.block.statements.items[s];
                            if (mstmt->type == AST_STMT_FN_DECL) {
                                emit_indent(b, indent);
                                buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                                           mstmt->as.fn_decl.name, t, mstmt->as.fn_decl.name);
                            } else if (mstmt->type == AST_STMT_CLASS_DECL) {
                                emit_indent(b, indent);
                                buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                                           mstmt->as.class_decl.name, t, mstmt->as.class_decl.name);
                            } else if (mstmt->type == AST_STMT_VAR_DECL) {
                                emit_indent(b, indent);
                                buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                                           mstmt->as.var_decl.name, t, mstmt->as.var_decl.name);
                            } else if (mstmt->type == AST_STMT_ASSIGN && mstmt->as.assign.target->type == AST_VARIABLE) {
                                emit_indent(b, indent);
                                buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                                           mstmt->as.assign.target->as.variable.name, t, mstmt->as.assign.target->as.variable.name);
                            }
                        }
                    } else {
                        for (size_t i = 0; i < stmt->as.from_import.count; ++i) {
                            FromImportItem* item = &stmt->as.from_import.items[i];
                            emit_indent(b, indent);
                            buf_printf(b, "sky_var_%s = dict_get(_mdict_%d, val_string(\"%s\"));\n",
                                       item->alias ? item->alias : item->symbol, t, item->symbol);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

static void emit_function_named(Buffer* b, AstNode* fn, const char* func_c_name, const char* func_display_name, const char* class_prefix) {
    scope_clear();
    const char* prev_fn = codegen_current_fn;
    codegen_current_fn = func_display_name;

    if (fn->as.fn_decl.is_async) {
        buf_printf(b, "static Value __real_%s(int argc, Value* argv) {\n", func_c_name);
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

        buf_printf(b, "typedef struct {\n");
        buf_printf(b, "    SkyFuture* future;\n");
        buf_printf(b, "    int argc;\n");
        buf_printf(b, "    Value* argv;\n");
        buf_printf(b, "} __AsyncArgs_%s;\n\n", func_c_name);

        buf_printf(b, "static void* __async_worker_%s(void* raw_args) {\n", func_c_name);
        buf_printf(b, "    __AsyncArgs_%s* a = (__AsyncArgs_%s*)raw_args;\n", func_c_name, func_c_name);
        buf_printf(b, "    Value res = __real_%s(a->argc, a->argv);\n", func_c_name);
        buf_printf(b, "    sky_future_resolve(a->future, res);\n");
        buf_printf(b, "    return NULL;\n");
        buf_printf(b, "}\n\n");

        buf_printf(b, "static Value %s(int argc, Value* argv) {\n", func_c_name);
        buf_printf(b, "    SkyFuture* fut = sky_future_create();\n");
        buf_printf(b, "    fut->has_thread = true;\n");
        buf_printf(b, "    __AsyncArgs_%s* args = (__AsyncArgs_%s*)GC_MALLOC(sizeof(__AsyncArgs_%s));\n", func_c_name, func_c_name, func_c_name);
        buf_printf(b, "    args->future = fut;\n");
        buf_printf(b, "    args->argc = argc;\n");
        buf_printf(b, "    args->argv = (argc > 0) ? (Value*)GC_MALLOC(sizeof(Value) * argc) : NULL;\n");
        buf_printf(b, "    for (int _i = 0; _i < argc; _i++) args->argv[_i] = argv[_i];\n");
        buf_printf(b, "    pthread_create(&fut->thread, NULL, __async_worker_%s, args);\n", func_c_name);
        buf_printf(b, "    return val_future(fut);\n");
        buf_printf(b, "}\n\n");
    } else {
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
    }
    codegen_current_fn = prev_fn;
}


char* codegen_emit_c(AstNode* root, const char* filename) {
    codegen_source_filename = filename;
    codegen_current_fn = "<main>";
    registry_init();

    load_modules_from_ast(&registry, root, filename);
    validate_foreign_scopes(&registry, root, filename);

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

    for (size_t m = 0; m < registry.count; ++m) {
        LoadedModule* mod = &registry.items[m];
        buf_printf(&b, "static Value sky_m%d_module_val = { .type = VAL_NIL };\n", mod->mod_id);
        buf_printf(&b, "static bool sky_m%d_module_initialized = false;\n", mod->mod_id);
        buf_printf(&b, "static Value sky_m%d_init(void);\n", mod->mod_id);

        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
            AstNode* stmt = mod->ast->as.block.statements.items[s];
            if (stmt->type == AST_STMT_FN_DECL) {
                buf_printf(&b, "static Value sky_m%d_fn_%s(int argc, Value* argv);\n", mod->mod_id, stmt->as.fn_decl.name);
            } else if (stmt->type == AST_STMT_CLASS_DECL) {
                for (size_t k = 0; k < stmt->as.class_decl.methods.count; ++k) {
                    AstNode* mfn = stmt->as.class_decl.methods.items[k];
                    buf_printf(&b, "static Value sky_m%d_method_%s_%s(int argc, Value* argv);\n",
                               mod->mod_id, stmt->as.class_decl.name, mfn->as.fn_decl.name);
                }
            }
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
    buf_puts(&b, "static Value sky_var_async;\n");

    const char* all_global_vars[4096];
    size_t global_vars_count = 0;

    for (size_t m = 0; m < registry.count; ++m) {
        LoadedModule* mod = &registry.items[m];
        add_global_id(mod->mod_name, all_global_vars, &global_vars_count, 4096);
        collect_global_identifiers(mod->ast, all_global_vars, &global_vars_count, 4096);
    }
    collect_global_identifiers(root, all_global_vars, &global_vars_count, 4096);

    for (size_t i = 0; i < global_vars_count; ++i) {
        buf_printf(&b, "static Value sky_var_%s;\n", all_global_vars[i]);
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

    /* Emit function & class method definitions for all loaded modules */
    for (size_t m = 0; m < registry.count; ++m) {
        LoadedModule* mod = &registry.items[m];
        codegen_source_filename = mod->filepath;
        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
            AstNode* stmt = mod->ast->as.block.statements.items[s];
            if (stmt->type == AST_STMT_FN_DECL) {
                char cname[256], dname[256];
                snprintf(cname, sizeof(cname), "sky_m%d_fn_%s", mod->mod_id, stmt->as.fn_decl.name);
                snprintf(dname, sizeof(dname), "%s.%s", mod->mod_name, stmt->as.fn_decl.name);
                emit_function_named(&b, stmt, cname, dname, NULL);
            } else if (stmt->type == AST_STMT_CLASS_DECL) {
                for (size_t k = 0; k < stmt->as.class_decl.methods.count; ++k) {
                    AstNode* mfn = stmt->as.class_decl.methods.items[k];
                    char cname[256], dname[256];
                    snprintf(cname, sizeof(cname), "sky_m%d_method_%s_%s", mod->mod_id, stmt->as.class_decl.name, mfn->as.fn_decl.name);
                    snprintf(dname, sizeof(dname), "%s.%s.%s", mod->mod_name, stmt->as.class_decl.name, mfn->as.fn_decl.name);
                    emit_function_named(&b, mfn, cname, dname, stmt->as.class_decl.name);
                }
            }
        }
    }

    /* Emit module initializer functions */
    for (size_t m = 0; m < registry.count; ++m) {
        LoadedModule* mod = &registry.items[m];
        codegen_source_filename = mod->filepath;
        buf_printf(&b, "static Value sky_m%d_init(void) {\n", mod->mod_id);
        buf_printf(&b, "    if (sky_m%d_module_initialized) return sky_m%d_module_val;\n", mod->mod_id, mod->mod_id);
        buf_printf(&b, "    sky_m%d_module_initialized = true;\n", mod->mod_id);
        if (mod->filepath) {
            buf_printf(&b, "    sky_push_frame(\"%s\", 1, \"<%s>\");\n", mod->filepath, mod->mod_name);
        }
        buf_puts(&b, "    Value mod_val = val_dict();\n");
        buf_puts(&b, "    ObjDict* _mod_dict = as_dict(mod_val);\n");
        buf_printf(&b, "    sky_m%d_module_val = mod_val;\n\n", mod->mod_id);

        /* 1. Process module-level imports */
        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
            AstNode* stmt = mod->ast->as.block.statements.items[s];
            if (stmt->type == AST_STMT_IMPORT || stmt->type == AST_STMT_FROM_IMPORT) {
                emit_statement(&b, stmt, NULL, 1);
            }
        }

        /* 2. Bind functions and classes to module dict and local globals */
        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
            AstNode* stmt = mod->ast->as.block.statements.items[s];
            if (stmt->type == AST_STMT_FN_DECL) {
                const char* fname = stmt->as.fn_decl.name;
                if (stmt->as.fn_decl.param_count > 0) {
                    buf_printf(&b, "    static const char* _params_m%d_%s[] = {", mod->mod_id, fname);
                    for (size_t p = 0; p < stmt->as.fn_decl.param_count; ++p) {
                        buf_printf(&b, "\"%s\"%s", stmt->as.fn_decl.params[p], (p + 1 < stmt->as.fn_decl.param_count) ? ", " : "");
                    }
                    buf_puts(&b, "};\n");
                    buf_printf(&b, "    Value _fn_m%d_%s = val_function_with_params(\"%s\", sky_m%d_fn_%s, %zu, %zu, _params_m%d_%s);\n",
                               mod->mod_id, fname, fname, mod->mod_id, fname, stmt->as.fn_decl.param_count, stmt->as.fn_decl.param_count, mod->mod_id, fname);
                } else {
                    buf_printf(&b, "    Value _fn_m%d_%s = val_function(\"%s\", sky_m%d_fn_%s, 0);\n",
                               mod->mod_id, fname, fname, mod->mod_id, fname);
                }
                buf_printf(&b, "    sky_var_%s = _fn_m%d_%s;\n", fname, mod->mod_id, fname);
                buf_printf(&b, "    dict_set(_mod_dict, val_string(\"%s\"), _fn_m%d_%s);\n", fname, mod->mod_id, fname);
            } else if (stmt->type == AST_STMT_CLASS_DECL) {
                const char* cname = stmt->as.class_decl.name;
                buf_printf(&b, "    Value _cls_val_m%d_%s = val_class(\"%s\");\n", mod->mod_id, cname, cname);
                buf_printf(&b, "    ObjClass* _cls_obj_m%d_%s = as_class(_cls_val_m%d_%s);\n", mod->mod_id, cname, mod->mod_id, cname);
                for (size_t k = 0; k < stmt->as.class_decl.methods.count; ++k) {
                    AstNode* mfn = stmt->as.class_decl.methods.items[k];
                    buf_puts(&b, "    {\n");
                    buf_puts(&b, "        MethodEntry* me = (MethodEntry*)GC_MALLOC(sizeof(MethodEntry));\n");
                    buf_printf(&b, "        me->name = \"%s\";\n", mfn->as.fn_decl.name);
                    if (mfn->as.fn_decl.param_count > 0) {
                        buf_printf(&b, "        static const char* _mparams_m%d_%s_%s[] = {", mod->mod_id, cname, mfn->as.fn_decl.name);
                        for (size_t p = 0; p < mfn->as.fn_decl.param_count; ++p) {
                            buf_printf(&b, "\"%s\"%s", mfn->as.fn_decl.params[p], (p + 1 < mfn->as.fn_decl.param_count) ? ", " : "");
                        }
                        buf_puts(&b, "};\n");
                        buf_printf(&b, "        me->fn = val_function_with_params(\"%s\", sky_m%d_method_%s_%s, %zu, %zu, _mparams_m%d_%s_%s);\n",
                                   mfn->as.fn_decl.name, mod->mod_id, cname, mfn->as.fn_decl.name, mfn->as.fn_decl.param_count + 1,
                                   mfn->as.fn_decl.param_count, mod->mod_id, cname, mfn->as.fn_decl.name);
                    } else {
                        buf_printf(&b, "        me->fn = val_function(\"%s\", sky_m%d_method_%s_%s, 0);\n",
                                   mfn->as.fn_decl.name, mod->mod_id, cname, mfn->as.fn_decl.name);
                    }
                    buf_printf(&b, "        me->next = _cls_obj_m%d_%s->methods;\n", mod->mod_id, cname);
                    buf_printf(&b, "        _cls_obj_m%d_%s->methods = me;\n", mod->mod_id, cname);
                    buf_puts(&b, "    }\n");
                }
                buf_printf(&b, "    sky_var_%s = _cls_val_m%d_%s;\n", cname, mod->mod_id, cname);
                buf_printf(&b, "    dict_set(_mod_dict, val_string(\"%s\"), _cls_val_m%d_%s);\n", cname, mod->mod_id, cname);
            }
        }

        /* 3. Execute non-decl statements and record exported module variables */
        scope_clear();
        for (size_t s = 0; s < mod->ast->as.block.statements.count; ++s) {
            AstNode* stmt = mod->ast->as.block.statements.items[s];
            if (stmt->type != AST_STMT_FN_DECL && stmt->type != AST_STMT_CLASS_DECL &&
                stmt->type != AST_STMT_EXTERN_DECL && stmt->type != AST_STMT_IMPORT &&
                stmt->type != AST_STMT_FROM_IMPORT) {
                emit_statement(&b, stmt, NULL, 1);
                if (stmt->type == AST_STMT_VAR_DECL) {
                    buf_printf(&b, "    dict_set(_mod_dict, val_string(\"%s\"), sky_var_%s);\n",
                               stmt->as.var_decl.name, stmt->as.var_decl.name);
                } else if (stmt->type == AST_STMT_ASSIGN && stmt->as.assign.target->type == AST_VARIABLE) {
                    buf_printf(&b, "    dict_set(_mod_dict, val_string(\"%s\"), sky_var_%s);\n",
                               stmt->as.assign.target->as.variable.name, stmt->as.assign.target->as.variable.name);
                } else if (stmt->type == AST_STMT_EXPR) {
                    AstNode* expr = stmt->as.expr_stmt.expr;
                    if (expr && expr->type == AST_METHOD_CALL && strcmp(expr->as.method_call.method_name, "exec") == 0) {
                        AstNode* target = expr->as.method_call.target;
                        AstNode* code_arg = expr->as.method_call.args.count > 0 ? expr->as.method_call.args.items[0] : NULL;
                        const char* code_str = (code_arg && code_arg->type == AST_LITERAL && code_arg->as.literal.lit_type == TOK_STRING_LIT)
                                               ? code_arg->as.literal.as.s_val : NULL;
                        if (target && target->type == AST_VARIABLE && code_str) {
                            ForeignSymbolTable syms;
                            foreign_symtable_init(&syms);
                            if (strcmp(target->as.variable.name, "js") == 0) {
                                sky_extract_js_declarations(code_str, mod->filepath, stmt->line, &syms);
                            } else if (strcmp(target->as.variable.name, "python") == 0) {
                                sky_extract_py_declarations(code_str, mod->filepath, stmt->line, &syms);
                            }
                            for (size_t si = 0; si < syms.count; ++si) {
                                buf_printf(&b, "    dict_set(_mod_dict, val_string(\"%s\"), sky_var_%s);\n",
                                           syms.items[si].name, syms.items[si].name);
                            }
                            foreign_symtable_free(&syms);
                        }
                    }
                }
            }
        }

        buf_puts(&b, "    sky_pop_frame();\n");
        buf_puts(&b, "    return mod_val;\n");
        buf_puts(&b, "}\n\n");
    }

    codegen_source_filename = filename;
    for (size_t i = 0; i < root->as.block.statements.count; ++i) {
        AstNode* stmt = root->as.block.statements.items[i];
        if (stmt->type == AST_STMT_FN_DECL) {
            char cname[256], dname[256];
            snprintf(cname, sizeof(cname), "sky_fn_%s", stmt->as.fn_decl.name);
            snprintf(dname, sizeof(dname), "%s", stmt->as.fn_decl.name);
            emit_function_named(&b, stmt, cname, dname, NULL);
        } else if (stmt->type == AST_STMT_CLASS_DECL) {
            for (size_t m = 0; m < stmt->as.class_decl.methods.count; ++m) {
                AstNode* mfn = stmt->as.class_decl.methods.items[m];
                char cname[256], dname[256];
                snprintf(cname, sizeof(cname), "sky_method_%s_%s", stmt->as.class_decl.name, mfn->as.fn_decl.name);
                snprintf(dname, sizeof(dname), "%s.%s", stmt->as.class_decl.name, mfn->as.fn_decl.name);
                emit_function_named(&b, mfn, cname, dname, stmt->as.class_decl.name);
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
    buf_puts(&b, "    sky_var_rust = sky_rust_init();\n");
    buf_puts(&b, "    sky_var_async = sky_mod_async;\n\n");

    for (size_t m = 0; m < registry.count; ++m) {
        LoadedModule* mod = &registry.items[m];
        buf_printf(&b, "    sky_var_%s = sky_m%d_init();\n", mod->mod_name, mod->mod_id);
    }

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
            stmt->type != AST_STMT_EXTERN_DECL && stmt->type != AST_STMT_CIMPORT) {
            emit_statement(&b, stmt, NULL, 1);
        }
    }

    buf_puts(&b, "\n    return 0;\n");
    buf_puts(&b, "}\n");

    return b.data;
}
