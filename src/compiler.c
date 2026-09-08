
#include "../include/sky_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void chunk_init(Chunk* chunk) {
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->lines = NULL;
    chunk->constants = NULL;
    chunk->const_count = 0;
    chunk->const_capacity = 0;
}

void chunk_write(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->count >= chunk->capacity) {
        chunk->capacity = chunk->capacity < 8 ? 8 : chunk->capacity * 2;
        chunk->code = (uint8_t*)realloc(chunk->code, chunk->capacity);
        chunk->lines = (int*)realloc(chunk->lines, sizeof(int) * chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

size_t chunk_add_constant(Chunk* chunk, Value value) {
    if (chunk->const_count >= chunk->const_capacity) {
        chunk->const_capacity = chunk->const_capacity < 8 ? 8 : chunk->const_capacity * 2;
        chunk->constants = (Value*)realloc(chunk->constants, sizeof(Value) * chunk->const_capacity);
    }
    chunk->constants[chunk->const_count] = value;
    return chunk->const_count++;
}

void chunk_free(Chunk* chunk) {
    free(chunk->code);
    free(chunk->lines);
    free(chunk->constants);
    chunk_init(chunk);
}

typedef struct {
    char* name;
    int depth;
    int slot;
} Local;

#define MAX_LOCALS 512

typedef struct {
    int loop_start;
    int* break_jumps;
    int break_count;
    int break_cap;
    int scope_depth;
} LoopCtx;

#define MAX_LOOP_DEPTH 32

typedef struct {
    CompiledFn* function;
    Local locals[MAX_LOCALS];
    int local_count;
    int scope_depth;

    LoopCtx loops[MAX_LOOP_DEPTH];
    int loop_depth;

    bool had_error;
} Compiler;

static void compiler_init(Compiler* c, const char* name) {
    c->function = (CompiledFn*)malloc(sizeof(CompiledFn));
    c->function->name = strdup(name);
    c->function->arity = 0;
    c->function->params = NULL;
    c->function->param_count = 0;
    c->function->local_count = 0;
    chunk_init(&c->function->chunk);
    c->local_count = 0;
    c->scope_depth = 0;
    c->loop_depth = 0;
    c->had_error = false;
}

static Chunk* current_chunk(Compiler* c) {
    return &c->function->chunk;
}

static void emit_byte(Compiler* c, uint8_t byte, int line) {
    chunk_write(current_chunk(c), byte, line);
}

static size_t emit_constant(Compiler* c, Value value, int line) {
    size_t idx = chunk_add_constant(current_chunk(c), value);
    emit_byte(c, OP_CONST, line);
    emit_byte(c, (uint8_t)((idx >> 8) & 0xFF), line);
    emit_byte(c, (uint8_t)(idx & 0xFF), line);
    return idx;
}

static size_t make_constant(Compiler* c, Value value) {
    return chunk_add_constant(current_chunk(c), value);
}

static void emit_16(Compiler* c, uint16_t val, int line) {
    emit_byte(c, (uint8_t)((val >> 8) & 0xFF), line);
    emit_byte(c, (uint8_t)(val & 0xFF), line);
}

static int emit_jump(Compiler* c, uint8_t opcode, int line) {
    emit_byte(c, opcode, line);
    emit_byte(c, 0xFF, line);
    emit_byte(c, 0xFF, line);
    return (int)current_chunk(c)->count - 2;
}

static void patch_jump(Compiler* c, int offset) {
    int jump = (int)current_chunk(c)->count - offset - 2;
    if (jump > 0xFFFF) {
        fprintf(stderr, "Compiler Error: Jump too large\n");
        c->had_error = true;
        return;
    }
    current_chunk(c)->code[offset]     = (uint8_t)((jump >> 8) & 0xFF);
    current_chunk(c)->code[offset + 1] = (uint8_t)(jump & 0xFF);
}

static void emit_loop(Compiler* c, int loop_start, int line) {
    emit_byte(c, OP_LOOP, line);
    int offset = (int)current_chunk(c)->count - loop_start + 2;
    if (offset > 0xFFFF) {
        fprintf(stderr, "Compiler Error: Loop body too large\n");
        c->had_error = true;
        return;
    }
    emit_byte(c, (uint8_t)((offset >> 8) & 0xFF), line);
    emit_byte(c, (uint8_t)(offset & 0xFF), line);
}

static void begin_scope(Compiler* c) {
    c->scope_depth++;
}

static void end_scope(Compiler* c, int line) {
    c->scope_depth--;
    while (c->local_count > 0 && c->locals[c->local_count - 1].depth > c->scope_depth) {
        emit_byte(c, OP_POP, line);
        c->local_count--;
    }
}

static int add_local(Compiler* c, const char* name) {
    if (c->local_count >= MAX_LOCALS) {
        fprintf(stderr, "Compiler Error: Too many local variables\n");
        c->had_error = true;
        return -1;
    }
    Local* local = &c->locals[c->local_count];
    local->name = strdup(name);
    local->depth = c->scope_depth;
    local->slot = c->local_count;
    c->local_count++;
    if (c->local_count > c->function->local_count)
        c->function->local_count = c->local_count;
    return local->slot;
}

static int resolve_local(Compiler* c, const char* name) {
    for (int i = c->local_count - 1; i >= 0; i--) {
        if (strcmp(c->locals[i].name, name) == 0) {
            return c->locals[i].slot;
        }
    }
    return -1;
}

static size_t name_constant(Compiler* c, const char* name) {
    return chunk_add_constant(current_chunk(c), val_string(name));
}

static void push_loop(Compiler* c, int loop_start) {
    if (c->loop_depth >= MAX_LOOP_DEPTH) return;
    LoopCtx* lc = &c->loops[c->loop_depth++];
    lc->loop_start = loop_start;
    lc->break_jumps = NULL;
    lc->break_count = 0;
    lc->break_cap = 0;
    lc->scope_depth = c->scope_depth;
}

static void add_break_jump(Compiler* c, int jump_offset) {
    if (c->loop_depth <= 0) return;
    LoopCtx* lc = &c->loops[c->loop_depth - 1];
    if (lc->break_count >= lc->break_cap) {
        lc->break_cap = lc->break_cap < 4 ? 4 : lc->break_cap * 2;
        lc->break_jumps = (int*)realloc(lc->break_jumps, sizeof(int) * lc->break_cap);
    }
    lc->break_jumps[lc->break_count++] = jump_offset;
}

static void pop_loop(Compiler* c) {
    if (c->loop_depth <= 0) return;
    LoopCtx* lc = &c->loops[--c->loop_depth];

    for (int i = 0; i < lc->break_count; i++) {
        patch_jump(c, lc->break_jumps[i]);
    }
    free(lc->break_jumps);
}

static void compile_expr(Compiler* c, AstNode* node);
static void compile_stmt(Compiler* c, AstNode* stmt);

static void compile_expr(Compiler* c, AstNode* node) {
    if (!node) { emit_byte(c, OP_NIL, 0); return; }
    int line = node->line;

    switch (node->type) {
        case AST_LITERAL: {
            switch (node->as.literal.lit_type) {
                case TOK_INT_LIT:
                    emit_constant(c, val_int(node->as.literal.as.i_val), line);
                    break;
                case TOK_DOUBLE_LIT:
                    emit_constant(c, val_double(node->as.literal.as.d_val), line);
                    break;
                case TOK_STRING_LIT:
                    emit_constant(c, val_string(node->as.literal.as.s_val), line);
                    break;
                case TOK_CHAR_LIT:
                    emit_constant(c, val_char(node->as.literal.as.c_val), line);
                    break;
                case TOK_KW_TRUE:
                    emit_byte(c, OP_TRUE, line);
                    break;
                case TOK_KW_FALSE:
                    emit_byte(c, OP_FALSE, line);
                    break;
                case TOK_KW_NIL:
                    emit_byte(c, OP_NIL, line);
                    break;
                default:
                    emit_byte(c, OP_NIL, line);
                    break;
            }
            break;
        }

        case AST_VARIABLE: {
            const char* name = node->as.variable.name;
            int slot = resolve_local(c, name);
            if (slot >= 0) {
                emit_byte(c, OP_GET_LOCAL, line);
                emit_16(c, (uint16_t)slot, line);
            } else {
                size_t idx = name_constant(c, name);
                emit_byte(c, OP_GET_GLOBAL, line);
                emit_16(c, (uint16_t)idx, line);
            }
            break;
        }

        case AST_THIS_VAR: {
            size_t idx = name_constant(c, node->as.variable.name);
            emit_byte(c, OP_GET_THIS, line);
            emit_16(c, (uint16_t)idx, line);
            break;
        }

        case AST_BINARY: {

            if (node->as.binary.op == TOK_AND) {
                compile_expr(c, node->as.binary.left);
                int jump = emit_jump(c, OP_JUMP_IF_FALSE, line);
                emit_byte(c, OP_POP, line);
                compile_expr(c, node->as.binary.right);
                patch_jump(c, jump);
                break;
            }
            if (node->as.binary.op == TOK_OR) {
                compile_expr(c, node->as.binary.left);
                int jump = emit_jump(c, OP_JUMP_IF_TRUE, line);
                emit_byte(c, OP_POP, line);
                compile_expr(c, node->as.binary.right);
                patch_jump(c, jump);
                break;
            }
            compile_expr(c, node->as.binary.left);
            compile_expr(c, node->as.binary.right);
            switch (node->as.binary.op) {
                case TOK_PLUS:     emit_byte(c, OP_ADD, line); break;
                case TOK_MINUS:    emit_byte(c, OP_SUB, line); break;
                case TOK_STAR:     emit_byte(c, OP_MUL, line); break;
                case TOK_SLASH:    emit_byte(c, OP_DIV, line); break;
                case TOK_FLOORDIV: emit_byte(c, OP_FLOORDIV, line); break;
                case TOK_PERCENT:  emit_byte(c, OP_MOD, line); break;
                case TOK_CARET:    emit_byte(c, OP_POW, line); break;
                case TOK_EQ:       emit_byte(c, OP_EQ, line); break;
                case TOK_NEQ:      emit_byte(c, OP_NEQ, line); break;
                case TOK_LT:       emit_byte(c, OP_LT, line); break;
                case TOK_LTE:      emit_byte(c, OP_LTE, line); break;
                case TOK_GT:       emit_byte(c, OP_GT, line); break;
                case TOK_GTE:      emit_byte(c, OP_GTE, line); break;
                default: break;
            }
            break;
        }

        case AST_UNARY: {
            compile_expr(c, node->as.unary.operand);
            if (node->as.unary.op == TOK_MINUS) emit_byte(c, OP_NEG, line);
            else if (node->as.unary.op == TOK_BANG) emit_byte(c, OP_NOT, line);
            break;
        }

        case AST_CALL: {
            compile_expr(c, node->as.call.callee);
            int argc = (int)node->as.call.args.count;
            for (int i = 0; i < argc; i++) {
                AstNode* arg = node->as.call.args.items[i];
                if (arg->type == AST_NAMED_ARG) {
                    compile_expr(c, arg->as.named_arg.value);
                } else {
                    compile_expr(c, arg);
                }
            }
            emit_byte(c, OP_CALL, line);
            emit_byte(c, (uint8_t)argc, line);
            break;
        }

        case AST_METHOD_CALL: {
            compile_expr(c, node->as.method_call.target);
            int argc = (int)node->as.method_call.args.count;
            for (int i = 0; i < argc; i++) {
                AstNode* arg = node->as.method_call.args.items[i];
                if (arg->type == AST_NAMED_ARG) {
                    compile_expr(c, arg->as.named_arg.value);
                } else {
                    compile_expr(c, arg);
                }
            }
            size_t idx = name_constant(c, node->as.method_call.method_name);
            emit_byte(c, OP_CALL_METHOD, line);
            emit_16(c, (uint16_t)idx, line);
            emit_byte(c, (uint8_t)argc, line);
            break;
        }

        case AST_PROP_GET: {
            compile_expr(c, node->as.prop.target);
            size_t idx = name_constant(c, node->as.prop.prop_name);
            emit_byte(c, OP_GET_PROP, line);
            emit_16(c, (uint16_t)idx, line);
            break;
        }

        case AST_INDEX_GET: {
            compile_expr(c, node->as.index.target);
            compile_expr(c, node->as.index.index);
            emit_byte(c, OP_GET_INDEX, line);
            break;
        }

        case AST_SLICE: {
            compile_expr(c, node->as.slice.target);
            if (node->as.slice.start)
                compile_expr(c, node->as.slice.start);
            else
                emit_byte(c, OP_NIL, line);
            if (node->as.slice.end)
                compile_expr(c, node->as.slice.end);
            else
                emit_byte(c, OP_NIL, line);
            emit_byte(c, OP_SLICE, line);
            break;
        }

        case AST_LIST_LIT: {
            int count = (int)node->as.collection.items.count;
            for (int i = 0; i < count; i++)
                compile_expr(c, node->as.collection.items.items[i]);
            emit_byte(c, OP_BUILD_LIST, line);
            emit_16(c, (uint16_t)count, line);
            break;
        }

        case AST_ARRAY_LIT: {
            int count = (int)node->as.collection.items.count;
            for (int i = 0; i < count; i++)
                compile_expr(c, node->as.collection.items.items[i]);
            emit_byte(c, OP_BUILD_ARRAY, line);
            emit_16(c, (uint16_t)count, line);
            break;
        }

        case AST_TUPLE_LIT: {
            int count = (int)node->as.collection.items.count;
            for (int i = 0; i < count; i++)
                compile_expr(c, node->as.collection.items.items[i]);
            emit_byte(c, OP_BUILD_TUPLE, line);
            emit_16(c, (uint16_t)count, line);
            break;
        }

        case AST_DICT_LIT: {
            int count = (int)node->as.dict_lit.keys.count;
            for (int i = 0; i < count; i++) {
                compile_expr(c, node->as.dict_lit.keys.items[i]);
                compile_expr(c, node->as.dict_lit.values.items[i]);
            }
            emit_byte(c, OP_BUILD_DICT, line);
            emit_16(c, (uint16_t)count, line);
            break;
        }

        case AST_SET_LIT: {
            int count = (int)node->as.collection.items.count;
            for (int i = 0; i < count; i++)
                compile_expr(c, node->as.collection.items.items[i]);
            emit_byte(c, OP_BUILD_SET, line);
            emit_16(c, (uint16_t)count, line);
            break;
        }

        case AST_STMT_ASSIGN: {
            compile_expr(c, node->as.assign.value);
            AstNode* target = node->as.assign.target;
            if (target->type == AST_VARIABLE) {
                int slot = resolve_local(c, target->as.variable.name);
                if (slot >= 0) {
                    emit_byte(c, OP_SET_LOCAL, line);
                    emit_16(c, (uint16_t)slot, line);
                } else {
                    size_t idx = name_constant(c, target->as.variable.name);
                    emit_byte(c, OP_SET_GLOBAL, line);
                    emit_16(c, (uint16_t)idx, line);
                }
            } else if (target->type == AST_THIS_VAR) {
                size_t idx = name_constant(c, target->as.variable.name);
                emit_byte(c, OP_SET_THIS, line);
                emit_16(c, (uint16_t)idx, line);
            }
            break;
        }

        case AST_INDEX_SET: {
            compile_expr(c, node->as.index.target);
            compile_expr(c, node->as.index.index);
            compile_expr(c, node->as.index.value);
            emit_byte(c, OP_SET_INDEX, line);
            break;
        }

        case AST_PROP_SET: {
            compile_expr(c, node->as.prop.value);
            compile_expr(c, node->as.prop.target);
            size_t idx = name_constant(c, node->as.prop.prop_name);
            emit_byte(c, OP_SET_PROP, line);
            emit_16(c, (uint16_t)idx, line);
            break;
        }

        default:
            emit_byte(c, OP_NIL, line);
            break;
    }
}

static void compile_stmt(Compiler* c, AstNode* stmt) {
    if (!stmt) return;
    int line = stmt->line;

    switch (stmt->type) {
        case AST_STMT_EXPR: {
            compile_expr(c, stmt->as.expr_stmt.expr);
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_STMT_VAR_DECL: {
            const char* name = stmt->as.var_decl.name;

            if (stmt->as.var_decl.init_expr) {
                compile_expr(c, stmt->as.var_decl.init_expr);
            } else {

                switch (stmt->as.var_decl.type_tok) {
                    case TOK_KW_I: emit_constant(c, val_int(0), line); break;
                    case TOK_KW_D: emit_constant(c, val_double(0.0), line); break;
                    case TOK_KW_B: emit_byte(c, OP_FALSE, line); break;
                    case TOK_KW_C: emit_constant(c, val_char('a'), line); break;
                    case TOK_KW_S: emit_constant(c, val_string(""), line); break;
                    case TOK_KW_L: {
                        emit_byte(c, OP_BUILD_LIST, line);
                        emit_16(c, 0, line);
                        break;
                    }
                    case TOK_KW_T: {
                        emit_byte(c, OP_BUILD_TUPLE, line);
                        emit_16(c, 0, line);
                        break;
                    }
                    case TOK_KW_DICT: {
                        emit_byte(c, OP_BUILD_DICT, line);
                        emit_16(c, 0, line);
                        break;
                    }
                    case TOK_KW_SET: {
                        emit_byte(c, OP_BUILD_SET, line);
                        emit_16(c, 0, line);
                        break;
                    }
                    case TOK_KW_SL: {

                        emit_byte(c, OP_BUILD_LIST, line);
                        emit_16(c, 0, line);
                        break;
                    }
                    default: emit_byte(c, OP_NIL, line); break;
                }
            }

            if (c->scope_depth > 0) {
                add_local(c, name);
            } else {
                size_t idx = name_constant(c, name);
                emit_byte(c, OP_SET_GLOBAL, line);
                emit_16(c, (uint16_t)idx, line);
                emit_byte(c, OP_POP, line);
            }
            break;
        }

        case AST_STMT_ASSIGN: {
            AstNode* target = stmt->as.assign.target;
            TokenType op = stmt->as.assign.op;

            if (op == TOK_PLUS_ASSIGN || op == TOK_MINUS_ASSIGN ||
                op == TOK_STAR_ASSIGN || op == TOK_SLASH_ASSIGN) {
                compile_expr(c, target);
                compile_expr(c, stmt->as.assign.value);
                switch (op) {
                    case TOK_PLUS_ASSIGN:  emit_byte(c, OP_ADD, line); break;
                    case TOK_MINUS_ASSIGN: emit_byte(c, OP_SUB, line); break;
                    case TOK_STAR_ASSIGN:  emit_byte(c, OP_MUL, line); break;
                    case TOK_SLASH_ASSIGN: emit_byte(c, OP_DIV, line); break;
                    default: break;
                }
            } else {
                compile_expr(c, stmt->as.assign.value);
            }

            if (target->type == AST_VARIABLE) {
                int slot = resolve_local(c, target->as.variable.name);
                if (slot >= 0) {
                    emit_byte(c, OP_SET_LOCAL, line);
                    emit_16(c, (uint16_t)slot, line);
                } else {
                    size_t idx = name_constant(c, target->as.variable.name);
                    emit_byte(c, OP_SET_GLOBAL, line);
                    emit_16(c, (uint16_t)idx, line);
                }
            } else if (target->type == AST_THIS_VAR) {
                size_t idx = name_constant(c, target->as.variable.name);
                emit_byte(c, OP_SET_THIS, line);
                emit_16(c, (uint16_t)idx, line);
            }
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_STMT_MULTI_VAR_DECL:
        case AST_STMT_MULTI_ASSIGN: {

            compile_expr(c, stmt->as.multi_assign.expr);

            for (size_t i = 0; i < stmt->as.multi_assign.count; i++) {
                emit_byte(c, OP_DUP, line);
                emit_byte(c, OP_UNPACK, line);
                emit_16(c, (uint16_t)i, line);

                const char* vname = stmt->as.multi_assign.names[i];
                if (strcmp(vname, "_") == 0) {
                    emit_byte(c, OP_POP, line);
                    continue;
                }
                if (stmt->type == AST_STMT_MULTI_VAR_DECL) {
                    if (c->scope_depth > 0) {
                        add_local(c, vname);
                    } else {
                        size_t idx = name_constant(c, vname);
                        emit_byte(c, OP_SET_GLOBAL, line);
                        emit_16(c, (uint16_t)idx, line);
                        emit_byte(c, OP_POP, line);
                    }
                } else {
                    int slot = resolve_local(c, vname);
                    if (slot >= 0) {
                        emit_byte(c, OP_SET_LOCAL, line);
                        emit_16(c, (uint16_t)slot, line);
                    } else {
                        size_t idx = name_constant(c, vname);
                        emit_byte(c, OP_SET_GLOBAL, line);
                        emit_16(c, (uint16_t)idx, line);
                    }
                    emit_byte(c, OP_POP, line);
                }
            }
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_STMT_RETURN: {
            if (stmt->as.ret.expr) {
                compile_expr(c, stmt->as.ret.expr);
            } else {
                emit_byte(c, OP_NIL, line);
            }
            emit_byte(c, OP_RETURN, line);
            break;
        }

        case AST_STMT_IF: {
            compile_expr(c, stmt->as.if_stmt.cond);
            int then_jump = emit_jump(c, OP_JUMP_IF_FALSE, line);
            emit_byte(c, OP_POP, line);

            if (stmt->as.if_stmt.then_branch->type == AST_STMT_BLOCK) {
                begin_scope(c);
                for (size_t i = 0; i < stmt->as.if_stmt.then_branch->as.block.statements.count; i++)
                    compile_stmt(c, stmt->as.if_stmt.then_branch->as.block.statements.items[i]);
                end_scope(c, line);
            } else {
                compile_stmt(c, stmt->as.if_stmt.then_branch);
            }

            int else_jump = emit_jump(c, OP_JUMP, line);
            patch_jump(c, then_jump);
            emit_byte(c, OP_POP, line);

            if (stmt->as.if_stmt.else_branch) {
                if (stmt->as.if_stmt.else_branch->type == AST_STMT_BLOCK) {
                    begin_scope(c);
                    for (size_t i = 0; i < stmt->as.if_stmt.else_branch->as.block.statements.count; i++)
                        compile_stmt(c, stmt->as.if_stmt.else_branch->as.block.statements.items[i]);
                    end_scope(c, line);
                } else {
                    compile_stmt(c, stmt->as.if_stmt.else_branch);
                }
            }
            patch_jump(c, else_jump);
            break;
        }

        case AST_STMT_WHILE: {
            int loop_start = (int)current_chunk(c)->count;
            push_loop(c, loop_start);

            compile_expr(c, stmt->as.while_stmt.cond);
            int exit_jump = emit_jump(c, OP_JUMP_IF_FALSE, line);
            emit_byte(c, OP_POP, line);

            if (stmt->as.while_stmt.body->type == AST_STMT_BLOCK) {
                begin_scope(c);
                for (size_t i = 0; i < stmt->as.while_stmt.body->as.block.statements.count; i++)
                    compile_stmt(c, stmt->as.while_stmt.body->as.block.statements.items[i]);
                end_scope(c, line);
            } else {
                compile_stmt(c, stmt->as.while_stmt.body);
            }

            emit_loop(c, loop_start, line);
            patch_jump(c, exit_jump);
            emit_byte(c, OP_POP, line);

            pop_loop(c);
            break;
        }

        case AST_STMT_FOR_IN: {

            compile_expr(c, stmt->as.for_in.iter_expr);
            begin_scope(c);
            int col_slot = add_local(c, "__iter_col__");
            (void)col_slot;

            emit_constant(c, val_int(0), line);
            int cnt_slot = add_local(c, "__iter_cnt__");

            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)col_slot, line);
            size_t size_name = name_constant(c, "size");
            emit_byte(c, OP_GET_PROP, line);
            emit_16(c, (uint16_t)size_name, line);
            int size_slot = add_local(c, "__iter_size__");

            int loop_start = (int)current_chunk(c)->count;
            push_loop(c, loop_start);

            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)cnt_slot, line);
            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)size_slot, line);
            emit_byte(c, OP_LT, line);

            int exit_jump = emit_jump(c, OP_JUMP_IF_FALSE, line);
            emit_byte(c, OP_POP, line);

            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)col_slot, line);
            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)cnt_slot, line);
            emit_byte(c, OP_GET_INDEX, line);
            int elem_slot = add_local(c, stmt->as.for_in.var_name);
            (void)elem_slot;

            if (stmt->as.for_in.body->type == AST_STMT_BLOCK) {
                begin_scope(c);
                for (size_t i = 0; i < stmt->as.for_in.body->as.block.statements.count; i++)
                    compile_stmt(c, stmt->as.for_in.body->as.block.statements.items[i]);
                end_scope(c, line);
            }

            emit_byte(c, OP_POP, line);
            c->local_count--;

            emit_byte(c, OP_GET_LOCAL, line);
            emit_16(c, (uint16_t)cnt_slot, line);
            emit_constant(c, val_int(1), line);
            emit_byte(c, OP_ADD, line);
            emit_byte(c, OP_SET_LOCAL, line);
            emit_16(c, (uint16_t)cnt_slot, line);
            emit_byte(c, OP_POP, line);

            emit_loop(c, loop_start, line);
            patch_jump(c, exit_jump);
            emit_byte(c, OP_POP, line);

            pop_loop(c);
            end_scope(c, line);
            break;
        }

        case AST_STMT_BREAK: {
            if (c->loop_depth > 0) {
                int jump = emit_jump(c, OP_JUMP, line);
                add_break_jump(c, jump);
            }
            break;
        }

        case AST_STMT_CONTINUE: {
            if (c->loop_depth > 0) {
                emit_loop(c, c->loops[c->loop_depth - 1].loop_start, line);
            }
            break;
        }

        case AST_STMT_BLOCK: {
            begin_scope(c);
            for (size_t i = 0; i < stmt->as.block.statements.count; i++)
                compile_stmt(c, stmt->as.block.statements.items[i]);
            end_scope(c, line);
            break;
        }

        case AST_STMT_FN_DECL: {

            Compiler fn_compiler;
            compiler_init(&fn_compiler, stmt->as.fn_decl.name);
            fn_compiler.function->arity = (int)stmt->as.fn_decl.param_count;
            fn_compiler.function->param_count = (int)stmt->as.fn_decl.param_count;
            if (stmt->as.fn_decl.param_count > 0) {
                fn_compiler.function->params = (char**)malloc(sizeof(char*) * stmt->as.fn_decl.param_count);
                for (size_t i = 0; i < stmt->as.fn_decl.param_count; i++) {
                    fn_compiler.function->params[i] = strdup(stmt->as.fn_decl.params[i]);
                }
            }

            begin_scope(&fn_compiler);

            for (size_t i = 0; i < stmt->as.fn_decl.param_count; i++) {
                add_local(&fn_compiler, stmt->as.fn_decl.params[i]);
            }

            AstNode* body = stmt->as.fn_decl.body;
            for (size_t i = 0; i < body->as.block.statements.count; i++) {
                compile_stmt(&fn_compiler, body->as.block.statements.items[i]);
            }

            emit_byte(&fn_compiler, OP_NIL, line);
            emit_byte(&fn_compiler, OP_RETURN, line);

            CompiledFn* fn = fn_compiler.function;

            Value fn_marker = val_int((int64_t)(intptr_t)fn);
            fn_marker.type = VAL_OBJ;

            size_t fn_idx = make_constant(c, fn_marker);
            emit_byte(c, OP_CONST, line);
            emit_16(c, (uint16_t)fn_idx, line);

            size_t name_idx = name_constant(c, stmt->as.fn_decl.name);
            emit_byte(c, OP_SET_GLOBAL, line);
            emit_16(c, (uint16_t)name_idx, line);
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_STMT_CLASS_DECL: {

            size_t name_idx = name_constant(c, stmt->as.class_decl.name);
            emit_byte(c, OP_CLASS, line);
            emit_16(c, (uint16_t)name_idx, line);

            size_t gname_idx = name_constant(c, stmt->as.class_decl.name);
            emit_byte(c, OP_SET_GLOBAL, line);
            emit_16(c, (uint16_t)gname_idx, line);
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_INDEX_SET: {
            compile_expr(c, stmt->as.index.target);
            compile_expr(c, stmt->as.index.index);
            compile_expr(c, stmt->as.index.value);
            emit_byte(c, OP_SET_INDEX, line);
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_PROP_SET: {
            compile_expr(c, stmt->as.prop.value);
            compile_expr(c, stmt->as.prop.target);
            size_t idx = name_constant(c, stmt->as.prop.prop_name);
            emit_byte(c, OP_SET_PROP, line);
            emit_16(c, (uint16_t)idx, line);
            emit_byte(c, OP_POP, line);
            break;
        }

        case AST_STMT_EXTERN_DECL:
        case AST_STMT_CIMPORT:
        case AST_STMT_IMPORT:
        case AST_STMT_FROM_IMPORT:
            break;

        default:

            compile_expr(c, stmt);
            emit_byte(c, OP_POP, line);
            break;
    }
}

CompiledFn* compile_ast(AstNode* program) {
    if (!program || program->type != AST_PROGRAM) return NULL;

    Compiler compiler;
    compiler_init(&compiler, "<script>");

    for (size_t i = 0; i < program->as.block.statements.count; i++) {
        compile_stmt(&compiler, program->as.block.statements.items[i]);
    }

    emit_byte(&compiler, OP_NIL, 0);
    emit_byte(&compiler, OP_RETURN, 0);

    if (compiler.had_error) {
        return NULL;
    }

    return compiler.function;
}

CompiledFn* compile_ast_eval(AstNode* program) {
    if (!program || program->type != AST_PROGRAM) return NULL;

    Compiler compiler;
    compiler_init(&compiler, "<eval>");

    size_t count = program->as.block.statements.count;
    if (count == 0) {
        emit_byte(&compiler, OP_NIL, 0);
        emit_byte(&compiler, OP_RETURN, 0);
        return compiler.function;
    }

    for (size_t i = 0; i < count - 1; i++) {
        compile_stmt(&compiler, program->as.block.statements.items[i]);
    }

    AstNode* last = program->as.block.statements.items[count - 1];
    if (last->type == AST_STMT_EXPR) {
        compile_expr(&compiler, last->as.expr_stmt.expr);
        emit_byte(&compiler, OP_RETURN, last->line);
    } else if (last->type == AST_STMT_RETURN) {
        compile_stmt(&compiler, last);
    } else if (last->type >= AST_LITERAL && last->type <= AST_SORTED_LIST_LIT) {
        compile_expr(&compiler, last);
        emit_byte(&compiler, OP_RETURN, last->line);
    } else {
        compile_stmt(&compiler, last);
        emit_byte(&compiler, OP_NIL, 0);
        emit_byte(&compiler, OP_RETURN, 0);
    }

    if (compiler.had_error) {
        return NULL;
    }

    return compiler.function;
}

