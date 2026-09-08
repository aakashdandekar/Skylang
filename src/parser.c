#include "../include/skylang.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;

    const char* fname = (parser->filename && parser->filename[0] != '\0') ? parser->filename : "<input>";
    fprintf(stderr, "  File \"%s\", line %d\n", fname, token->line);

    if (parser->lexer.source) {
        const char* p = parser->lexer.source;
        int cur_line = 1;
        while (*p && cur_line < token->line) {
            if (*p == '\n') cur_line++;
            p++;
        }
        const char* start = p;
        while (*p && *p != '\n' && *p != '\r') p++;
        size_t len = p - start;
        fprintf(stderr, "    %.*s\n", (int)len, start);

        int col = token->col > 0 ? token->col : 1;
        fprintf(stderr, "    ");
        for (int i = 1; i < col; ++i) fprintf(stderr, " ");
        fprintf(stderr, "^\n");
    }

    if (token->type == TOK_EOF) {
        fprintf(stderr, "SyntaxError: %s (at end of file)\n", message);
    } else if (token->type != TOK_ERROR && token->start && token->length > 0) {
        fprintf(stderr, "SyntaxError: %s (near '%.*s')\n", message, (int)token->length, token->start);
    } else {
        fprintf(stderr, "SyntaxError: %s\n", message);
    }
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = lexer_next_token(&parser->lexer);
        if (parser->current.type != TOK_ERROR) break;
        error_at(parser, &parser->current, parser->current.as.s_val);
    }
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error_at(parser, &parser->current, message);
}

void parser_init(Parser* parser, const char* source, const char* filename) {
    lexer_init(&parser->lexer, source);
    parser->had_error = false;
    parser->panic_mode = false;
    parser->angle_bracket_depth = 0;
    parser->filename = filename;
    advance(parser);
}

static AstNode* ast_new(AstNodeType type, int line) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    memset(node, 0, sizeof(AstNode));
    node->type = type;
    node->line = line;
    return node;
}

static void node_list_init(AstNodeList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void node_list_add(AstNodeList* list, AstNode* node) {
    if (list->count >= list->capacity) {
        list->capacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        list->items = (AstNode**)realloc(list->items, sizeof(AstNode*) * list->capacity);
    }
    list->items[list->count++] = node;
}

static AstNode* parse_statement(Parser* parser);
static AstNode* parse_expression(Parser* parser);
static AstNode* parse_call_arg(Parser* parser);
static AstNode* parse_block(Parser* parser);

static AstNode* parse_assignment(Parser* parser);
static AstNode* parse_logic_or(Parser* parser);
static AstNode* parse_logic_and(Parser* parser);
static AstNode* parse_equality(Parser* parser);
static AstNode* parse_comparison(Parser* parser);
static AstNode* parse_term(Parser* parser);
static AstNode* parse_factor(Parser* parser);
static AstNode* parse_power(Parser* parser);
static AstNode* parse_unary(Parser* parser);
static AstNode* parse_postfix(Parser* parser);
static AstNode* parse_primary(Parser* parser);

static AstNode* parse_primary(Parser* parser) {
    int line = parser->current.line;

    if (match(parser, TOK_INT_LIT)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_INT_LIT;
        n->as.literal.as.i_val = parser->previous.as.i_val;
        return n;
    }

    if (match(parser, TOK_DOUBLE_LIT)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_DOUBLE_LIT;
        n->as.literal.as.d_val = parser->previous.as.d_val;
        return n;
    }

    if (match(parser, TOK_STRING_LIT)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_STRING_LIT;
        n->as.literal.as.s_val = strdup(parser->previous.as.s_val);
        return n;
    }

    if (match(parser, TOK_CHAR_LIT)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_CHAR_LIT;
        n->as.literal.as.c_val = parser->previous.as.c_val;
        return n;
    }

    if (match(parser, TOK_KW_TRUE)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_KW_TRUE;
        n->as.literal.as.b_val = true;
        return n;
    }
    if (match(parser, TOK_KW_FALSE)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_KW_FALSE;
        n->as.literal.as.b_val = false;
        return n;
    }
    if (match(parser, TOK_KW_NIL)) {
        AstNode* n = ast_new(AST_LITERAL, line);
        n->as.literal.lit_type = TOK_KW_NIL;
        return n;
    }

    if (match(parser, TOK_KW_THIS)) {
        if (match(parser, TOK_DOT)) {
            consume(parser, TOK_IDENT, "Expect property name after 'this.'");
            AstNode* n = ast_new(AST_THIS_VAR, line);
            n->as.variable.name = strdup(parser->previous.as.s_val);
            return n;
        }
        AstNode* n = ast_new(AST_VARIABLE, line);
        n->as.variable.name = strdup("this");
        return n;
    }

    if (match(parser, TOK_LT)) {
        AstNode* n = ast_new(AST_ARRAY_LIT, line);
        node_list_init(&n->as.collection.items);
        parser->angle_bracket_depth++;
        if (!check(parser, TOK_GT)) {
            do {
                node_list_add(&n->as.collection.items, parse_expression(parser));
            } while (match(parser, TOK_COMMA));
        }
        parser->angle_bracket_depth--;
        consume(parser, TOK_GT, "Expect '>' after array elements");
        return n;
    }

    if (match(parser, TOK_LBRACKET)) {
        AstNode* n = ast_new(AST_LIST_LIT, line);
        node_list_init(&n->as.collection.items);
        if (!check(parser, TOK_RBRACKET)) {
            do {
                node_list_add(&n->as.collection.items, parse_expression(parser));
            } while (match(parser, TOK_COMMA));
        }
        consume(parser, TOK_RBRACKET, "Expect ']' after list elements");
        return n;
    }

    if (match(parser, TOK_LBRACE)) {
        if (match(parser, TOK_RBRACE)) {

            AstNode* n = ast_new(AST_DICT_LIT, line);
            node_list_init(&n->as.dict_lit.keys);
            node_list_init(&n->as.dict_lit.values);
            return n;
        }
        AstNode* first_expr = parse_expression(parser);
        if (match(parser, TOK_COLON)) {

            AstNode* first_val = parse_expression(parser);
            AstNode* n = ast_new(AST_DICT_LIT, line);
            node_list_init(&n->as.dict_lit.keys);
            node_list_init(&n->as.dict_lit.values);
            node_list_add(&n->as.dict_lit.keys, first_expr);
            node_list_add(&n->as.dict_lit.values, first_val);
            while (match(parser, TOK_COMMA)) {
                if (check(parser, TOK_RBRACE)) break;
                AstNode* k = parse_expression(parser);
                consume(parser, TOK_COLON, "Expect ':' after dict key");
                AstNode* v = parse_expression(parser);
                node_list_add(&n->as.dict_lit.keys, k);
                node_list_add(&n->as.dict_lit.values, v);
            }
            consume(parser, TOK_RBRACE, "Expect '}' after dict entries");
            return n;
        } else {

            AstNode* n = ast_new(AST_SET_LIT, line);
            node_list_init(&n->as.collection.items);
            node_list_add(&n->as.collection.items, first_expr);
            while (match(parser, TOK_COMMA)) {
                if (check(parser, TOK_RBRACE)) break;
                node_list_add(&n->as.collection.items, parse_expression(parser));
            }
            consume(parser, TOK_RBRACE, "Expect '}' after set items");
            return n;
        }
    }

    if (match(parser, TOK_LPAREN)) {
        if (match(parser, TOK_RPAREN)) {

            AstNode* n = ast_new(AST_TUPLE_LIT, line);
            node_list_init(&n->as.collection.items);
            return n;
        }
        AstNode* expr = parse_expression(parser);
        if (match(parser, TOK_COMMA)) {

            AstNode* n = ast_new(AST_TUPLE_LIT, line);
            node_list_init(&n->as.collection.items);
            node_list_add(&n->as.collection.items, expr);
            if (!check(parser, TOK_RPAREN)) {
                do {
                    node_list_add(&n->as.collection.items, parse_expression(parser));
                } while (match(parser, TOK_COMMA));
            }
            consume(parser, TOK_RPAREN, "Expect ')' after tuple elements");
            return n;
        }
        consume(parser, TOK_RPAREN, "Expect ')' after expression");
        return expr;
    }

    if (match(parser, TOK_KW_TAKES)) {
        consume(parser, TOK_LPAREN, "Expect '(' after 'takes'");
        AstNode* n = ast_new(AST_CALL, line);
        AstNode* callee = ast_new(AST_VARIABLE, line);
        callee->as.variable.name = strdup("takes");
        n->as.call.callee = callee;
        node_list_init(&n->as.call.args);
        if (!check(parser, TOK_RPAREN)) {
            do {
                node_list_add(&n->as.call.args, parse_call_arg(parser));
            } while (match(parser, TOK_COMMA));
        }
        consume(parser, TOK_RPAREN, "Expect ')' after takes parameters");
        return n;
    }

    if (match(parser, TOK_IDENT)) {
        AstNode* n = ast_new(AST_VARIABLE, line);
        n->as.variable.name = strdup(parser->previous.as.s_val);
        return n;
    }

    error_at(parser, &parser->current, "Expect expression");
    advance(parser);
    return ast_new(AST_LITERAL, line);
}

static AstNode* parse_call_arg(Parser* parser) {
    AstNode* expr = parse_expression(parser);
    if (expr && expr->type == AST_STMT_ASSIGN &&
        expr->as.assign.target->type == AST_VARIABLE &&
        expr->as.assign.op == TOK_ASSIGN) {
        AstNode* named = ast_new(AST_NAMED_ARG, expr->line);
        named->as.named_arg.name = strdup(expr->as.assign.target->as.variable.name);
        named->as.named_arg.value = expr->as.assign.value;
        return named;
    }
    return expr;
}

static AstNode* parse_postfix(Parser* parser) {
    AstNode* expr = parse_primary(parser);

    for (;;) {
        int line = parser->current.line;

        if (match(parser, TOK_LPAREN)) {
            AstNode* call = ast_new(AST_CALL, line);
            call->as.call.callee = expr;
            node_list_init(&call->as.call.args);
            if (!check(parser, TOK_RPAREN)) {
                do {
                    node_list_add(&call->as.call.args, parse_call_arg(parser));
                } while (match(parser, TOK_COMMA));
            }
            consume(parser, TOK_RPAREN, "Expect ')' after arguments");
            expr = call;
            continue;
        }

        if (match(parser, TOK_LBRACKET)) {
            AstNode* start = NULL;
            if (!check(parser, TOK_COLON)) {
                start = parse_expression(parser);
            }
            if (match(parser, TOK_COLON)) {

                AstNode* end = NULL;
                if (!check(parser, TOK_RBRACKET)) {
                    end = parse_expression(parser);
                }
                consume(parser, TOK_RBRACKET, "Expect ']' after slice");
                AstNode* slice = ast_new(AST_SLICE, line);
                slice->as.slice.target = expr;
                slice->as.slice.start = start;
                slice->as.slice.end = end;
                expr = slice;
            } else {

                consume(parser, TOK_RBRACKET, "Expect ']' after index");
                AstNode* idx = ast_new(AST_INDEX_GET, line);
                idx->as.index.target = expr;
                idx->as.index.index = start;
                expr = idx;
            }
            continue;
        }

        if (match(parser, TOK_DOT)) {
            char* name = NULL;
            if (match(parser, TOK_KW_T)) {
                name = strdup("T");
            } else if (match(parser, TOK_IDENT)) {
                name = strdup(parser->previous.as.s_val);
            } else {
                error_at(parser, &parser->current, "Expect property or method name after '.'");
                break;
            }

            if (match(parser, TOK_LPAREN)) {
                AstNode* mc = ast_new(AST_METHOD_CALL, line);
                mc->as.method_call.target = expr;
                mc->as.method_call.method_name = name;
                node_list_init(&mc->as.method_call.args);
                if (!check(parser, TOK_RPAREN)) {
                    do {
                        node_list_add(&mc->as.method_call.args, parse_call_arg(parser));
                    } while (match(parser, TOK_COMMA));
                }
                consume(parser, TOK_RPAREN, "Expect ')' after method arguments");
                expr = mc;
            } else {

                AstNode* prop = ast_new(AST_PROP_GET, line);
                prop->as.prop.target = expr;
                prop->as.prop.prop_name = name;
                expr = prop;
            }
            continue;
        }

        break;
    }

    return expr;
}

static AstNode* parse_unary(Parser* parser) {
    if (match(parser, TOK_BANG) || match(parser, TOK_MINUS)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* operand = parse_unary(parser);
        AstNode* n = ast_new(AST_UNARY, line);
        n->as.unary.op = op;
        n->as.unary.operand = operand;
        return n;
    }
    return parse_postfix(parser);
}

static AstNode* parse_power(Parser* parser) {
    AstNode* expr = parse_unary(parser);
    if (match(parser, TOK_CARET)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_power(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        return n;
    }
    return expr;
}

static AstNode* parse_factor(Parser* parser) {
    AstNode* expr = parse_power(parser);
    while (match(parser, TOK_STAR) || match(parser, TOK_SLASH) || match(parser, TOK_FLOORDIV) || match(parser, TOK_PERCENT)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_power(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_term(Parser* parser) {
    AstNode* expr = parse_factor(parser);
    while (match(parser, TOK_PLUS) || match(parser, TOK_MINUS)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_factor(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_comparison(Parser* parser) {
    AstNode* expr = parse_term(parser);
    while (check(parser, TOK_LT) || check(parser, TOK_LTE) ||
           (!parser->angle_bracket_depth && check(parser, TOK_GT)) ||
           check(parser, TOK_GTE)) {
        advance(parser);
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_term(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_equality(Parser* parser) {
    AstNode* expr = parse_comparison(parser);
    while (match(parser, TOK_EQ) || match(parser, TOK_NEQ)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_comparison(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_logic_and(Parser* parser) {
    AstNode* expr = parse_equality(parser);
    while (match(parser, TOK_AND)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_equality(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_logic_or(Parser* parser) {
    AstNode* expr = parse_logic_and(parser);
    while (match(parser, TOK_OR)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* right = parse_logic_and(parser);
        AstNode* n = ast_new(AST_BINARY, line);
        n->as.binary.op = op;
        n->as.binary.left = expr;
        n->as.binary.right = right;
        expr = n;
    }
    return expr;
}

static AstNode* parse_assignment(Parser* parser) {
    AstNode* expr = parse_logic_or(parser);

    if (match(parser, TOK_ASSIGN) || match(parser, TOK_WALRUS) ||
        match(parser, TOK_PLUS_ASSIGN) || match(parser, TOK_MINUS_ASSIGN) ||
        match(parser, TOK_STAR_ASSIGN) || match(parser, TOK_SLASH_ASSIGN)) {
        TokenType op = parser->previous.type;
        int line = parser->previous.line;
        AstNode* value = parse_assignment(parser);

        if (expr->type == AST_VARIABLE || expr->type == AST_THIS_VAR) {
            AstNode* assign = ast_new(AST_STMT_ASSIGN, line);
            assign->as.assign.target = expr;
            assign->as.assign.op = op;
            assign->as.assign.value = value;
            return assign;
        } else if (expr->type == AST_INDEX_GET) {
            AstNode* set_idx = ast_new(AST_INDEX_SET, line);
            set_idx->as.index.target = expr->as.index.target;
            set_idx->as.index.index = expr->as.index.index;
            set_idx->as.index.value = value;
            return set_idx;
        } else if (expr->type == AST_PROP_GET) {
            AstNode* set_prop = ast_new(AST_PROP_SET, line);
            set_prop->as.prop.target = expr->as.prop.target;
            set_prop->as.prop.prop_name = expr->as.prop.prop_name;
            set_prop->as.prop.value = value;
            return set_prop;
        }
        error_at(parser, &parser->previous, "Invalid assignment target");
    }

    return expr;
}

static AstNode* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

static AstNode* parse_block(Parser* parser) {
    int line = parser->current.line;
    consume(parser, TOK_LBRACE, "Expect '{' before block");
    AstNode* block = ast_new(AST_STMT_BLOCK, line);
    node_list_init(&block->as.block.statements);

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        AstNode* stmt = parse_statement(parser);
        if (stmt) {
            node_list_add(&block->as.block.statements, stmt);
        }
    }
    consume(parser, TOK_RBRACE, "Expect '}' after block");
    return block;
}

static AstNode* parse_function_decl(Parser* parser, const char* forced_name) {
    int line = parser->current.line;
    char* name = NULL;
    if (forced_name) {
        name = strdup(forced_name);
    } else {
        consume(parser, TOK_IDENT, "Expect function name");
        name = strdup(parser->previous.as.s_val);
    }

    if (match(parser, TOK_LPAREN)) {
        error_at(parser, &parser->previous,
            "Function and method definitions cannot have circular brackets '()'. Use 'takes(...)' inside body instead.");
        while (!check(parser, TOK_RPAREN) && !check(parser, TOK_EOF)) {
            advance(parser);
        }
        consume(parser, TOK_RPAREN, "Expect ')' after parameters");
    }

    char** params = NULL;
    size_t param_count = 0;
    size_t param_cap = 0;

    AstNode* body = parse_block(parser);

    for (size_t si = 0; si < body->as.block.statements.count; ++si) {
        AstNode* stmt = body->as.block.statements.items[si];
        if (stmt->type == AST_STMT_EXPR &&
            stmt->as.expr_stmt.expr->type == AST_CALL &&
            stmt->as.expr_stmt.expr->as.call.callee->type == AST_VARIABLE &&
            strcmp(stmt->as.expr_stmt.expr->as.call.callee->as.variable.name, "takes") == 0) {
            AstNodeList* args = &stmt->as.expr_stmt.expr->as.call.args;
            param_cap = args->count;
            params = (char**)malloc(sizeof(char*) * (param_cap > 0 ? param_cap : 1));
            for (size_t i = 0; i < args->count; ++i) {
                if (args->items[i]->type == AST_VARIABLE) {
                    params[param_count++] = strdup(args->items[i]->as.variable.name);
                } else if (args->items[i]->type == AST_NAMED_ARG) {
                    params[param_count++] = strdup(args->items[i]->as.named_arg.name);
                }
            }
            break;
        }
    }

    AstNode* fn = ast_new(AST_STMT_FN_DECL, line);
    fn->as.fn_decl.name = name;
    fn->as.fn_decl.params = params;
    fn->as.fn_decl.param_count = param_count;
    fn->as.fn_decl.body = body;
    return fn;
}

static AstNode* parse_class_decl(Parser* parser) {
    int line = parser->previous.line;
    consume(parser, TOK_IDENT, "Expect class name");
    char* name = strdup(parser->previous.as.s_val);

    consume(parser, TOK_LBRACE, "Expect '{' before class body");

    char** fields = NULL;
    size_t field_count = 0;
    size_t field_cap = 0;

    AstNodeList methods;
    node_list_init(&methods);

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {

        while (match(parser, TOK_SEMICOLON));

        if (check(parser, TOK_RBRACE)) break;

        if (match(parser, TOK_KW_THIS)) {
            consume(parser, TOK_DOT, "Expect '.' after this in class body");
            consume(parser, TOK_IDENT, "Expect field name after 'this.'");
            char* fname = strdup(parser->previous.as.s_val);
            if (field_count >= field_cap) {
                field_cap = (field_cap == 0) ? 4 : field_cap * 2;
                fields = (char**)realloc(fields, sizeof(char*) * field_cap);
            }
            fields[field_count++] = fname;
            match(parser, TOK_SEMICOLON);
            continue;
        }

        if (match(parser, TOK_KW_INIT)) {
            AstNode* init_fn = parse_function_decl(parser, "init");
            node_list_add(&methods, init_fn);
            continue;
        }

        if (match(parser, TOK_IDENT)) {
            char* mname = parser->previous.as.s_val;
            AstNode* method_fn = parse_function_decl(parser, mname);
            node_list_add(&methods, method_fn);
            continue;
        }

        error_at(parser, &parser->current, "Expect field or method in class body");
        advance(parser);
    }

    consume(parser, TOK_RBRACE, "Expect '}' after class body");

    AstNode* cls = ast_new(AST_STMT_CLASS_DECL, line);
    cls->as.class_decl.name = name;
    cls->as.class_decl.fields = fields;
    cls->as.class_decl.field_count = field_count;
    cls->as.class_decl.methods = methods;
    return cls;
}

static AstNode* parse_if_stmt(Parser* parser) {
    int line = parser->previous.line;
    AstNode* n = ast_new(AST_STMT_IF, line);
    n->as.if_stmt.cond = parse_expression(parser);
    n->as.if_stmt.then_branch = parse_block(parser);

    if (match(parser, TOK_KW_ELIF)) {
        n->as.if_stmt.else_branch = parse_if_stmt(parser);
    } else if (match(parser, TOK_KW_ELSE)) {
        if (match(parser, TOK_KW_IF) || match(parser, TOK_KW_ELIF)) {
            n->as.if_stmt.else_branch = parse_if_stmt(parser);
        } else {
            n->as.if_stmt.else_branch = parse_block(parser);
        }
    } else {
        n->as.if_stmt.else_branch = NULL;
    }
    return n;
}

static bool is_for_in_syntax(Parser* parser) {
    if (parser->current.type != TOK_IDENT) return false;
    const char* p = parser->lexer.current;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p[0] == 'i' && p[1] == 'n' && !isalnum(p[2]) && p[2] != '_') {
        return true;
    }
    return false;
}

static AstNode* parse_statement(Parser* parser) {
    int line = parser->current.line;

    while (match(parser, TOK_SEMICOLON));
    if (check(parser, TOK_EOF)) return NULL;

    if (match(parser, TOK_KW_IMPORT)) {
        AstNode* n = ast_new(AST_STMT_IMPORT, line);
        char** mnames = NULL;
        size_t mcount = 0, mcap = 0;

        do {
            char* mname = NULL;
            if (match(parser, TOK_IDENT)) {
                char buf[512];
                snprintf(buf, sizeof(buf), "%s", parser->previous.as.s_val);
                while (match(parser, TOK_DOT)) {
                    consume(parser, TOK_IDENT, "Expect identifier after '.' in import module path");
                    size_t cur_len = strlen(buf);
                    snprintf(buf + cur_len, sizeof(buf) - cur_len, "/%s", parser->previous.as.s_val);
                }
                mname = strdup(buf);
            } else if (match(parser, TOK_STRING_LIT)) {
                mname = strdup(parser->previous.as.s_val);
            } else {
                error_at(parser, &parser->current, "Expect module or file path after 'import'");
                return NULL;
            }

            if (mcount >= mcap) {
                mcap = mcap < 4 ? 4 : mcap * 2;
                mnames = realloc(mnames, mcap * sizeof(char*));
            }
            mnames[mcount++] = mname;
        } while (match(parser, TOK_COMMA));

        n->as.import_stmt.module_names = mnames;
        n->as.import_stmt.count = mcount;
        n->as.import_stmt.module_name = mcount > 0 ? mnames[0] : NULL;

        match(parser, TOK_SEMICOLON);
        return n;
    }

    if (match(parser, TOK_KW_EXTERN)) {
        consume(parser, TOK_KW_F, "Expect 'f' after 'extern'");
        consume(parser, TOK_IDENT, "Expect function name after 'extern f'");
        char* ename = strdup(parser->previous.as.s_val);
        consume(parser, TOK_LPAREN, "Expect '(' after extern function name");
        char** eparams = NULL;
        size_t epcount = 0, epcap = 0;
        if (!check(parser, TOK_RPAREN)) {
            do {
                consume(parser, TOK_IDENT, "Expect parameter name");
                if (epcount >= epcap) {
                    epcap = epcap ? epcap * 2 : 4;
                    eparams = (char**)realloc(eparams, sizeof(char*) * epcap);
                }
                eparams[epcount++] = strdup(parser->previous.as.s_val);
            } while (match(parser, TOK_COMMA));
        }
        consume(parser, TOK_RPAREN, "Expect ')' after extern parameters");
        match(parser, TOK_SEMICOLON);
        AstNode* n = ast_new(AST_STMT_EXTERN_DECL, line);
        n->as.extern_decl.name = ename;
        n->as.extern_decl.params = eparams;
        n->as.extern_decl.param_count = epcount;
        return n;
    }

    if (match(parser, TOK_KW_F)) {
        return parse_function_decl(parser, NULL);
    }

    if (match(parser, TOK_KW_CLASS)) {
        return parse_class_decl(parser);
    }

    if (match(parser, TOK_KW_RETURN)) {
        AstNode* n = ast_new(AST_STMT_RETURN, line);
        if (check(parser, TOK_SEMICOLON) || check(parser, TOK_RBRACE) || check(parser, TOK_EOF)) {
            n->as.ret.expr = NULL;
        } else {
            AstNode* first_expr = parse_expression(parser);
            if (match(parser, TOK_COMMA)) {
                AstNode* tuple_node = ast_new(AST_TUPLE_LIT, line);
                node_list_init(&tuple_node->as.collection.items);
                node_list_add(&tuple_node->as.collection.items, first_expr);
                do {
                    node_list_add(&tuple_node->as.collection.items, parse_expression(parser));
                } while (match(parser, TOK_COMMA));
                n->as.ret.expr = tuple_node;
            } else {
                n->as.ret.expr = first_expr;
            }
        }
        match(parser, TOK_SEMICOLON);
        return n;
    }

    if (match(parser, TOK_KW_IF)) {
        return parse_if_stmt(parser);
    }

    if (match(parser, TOK_KW_WHILE)) {
        AstNode* n = ast_new(AST_STMT_WHILE, line);
        n->as.while_stmt.cond = parse_expression(parser);
        n->as.while_stmt.body = parse_block(parser);
        return n;
    }

    if (match(parser, TOK_KW_FOR)) {
        if (check(parser, TOK_LBRACE)) {
            AstNode* n = ast_new(AST_STMT_WHILE, line);
            AstNode* true_cond = ast_new(AST_LITERAL, line);
            true_cond->as.literal.lit_type = TOK_KW_TRUE;
            true_cond->as.literal.as.b_val = true;
            n->as.while_stmt.cond = true_cond;
            n->as.while_stmt.body = parse_block(parser);
            return n;
        }
        if (is_for_in_syntax(parser)) {
            AstNode* n = ast_new(AST_STMT_FOR_IN, line);
            consume(parser, TOK_IDENT, "Expect variable name in for loop");
            n->as.for_in.var_name = strdup(parser->previous.as.s_val);
            consume(parser, TOK_KW_IN, "Expect 'in' after for variable");
            n->as.for_in.iter_expr = parse_expression(parser);
            n->as.for_in.body = parse_block(parser);
            return n;
        }

        AstNode* n = ast_new(AST_STMT_WHILE, line);
        n->as.while_stmt.cond = parse_expression(parser);
        n->as.while_stmt.body = parse_block(parser);
        return n;
    }

    if (match(parser, TOK_KW_BREAK)) {
        match(parser, TOK_SEMICOLON);
        return ast_new(AST_STMT_BREAK, line);
    }
    if (match(parser, TOK_KW_CONTINUE)) {
        match(parser, TOK_SEMICOLON);
        return ast_new(AST_STMT_CONTINUE, line);
    }

    if (match(parser, TOK_KW_I) || match(parser, TOK_KW_D) ||
        match(parser, TOK_KW_B) || match(parser, TOK_KW_C) ||
        match(parser, TOK_KW_S)) {
        TokenType type_tok = parser->previous.type;
        consume(parser, TOK_IDENT, "Expect variable name after type");
        char* name = strdup(parser->previous.as.s_val);
        AstNode* init = NULL;
        if (match(parser, TOK_ASSIGN)) {
            init = parse_expression(parser);
        }
        match(parser, TOK_SEMICOLON);

        AstNode* n = ast_new(AST_STMT_VAR_DECL, line);
        n->as.var_decl.name = name;
        n->as.var_decl.type_tok = type_tok;
        n->as.var_decl.size_expr = NULL;
        n->as.var_decl.init_expr = init;
        n->as.var_decl.is_walrus = false;
        return n;
    }

    if (check(parser, TOK_IDENT)) {
        Token ident_tok = parser->current;
        advance(parser);
        char* name = strdup(ident_tok.as.s_val);

        if (check(parser, TOK_COMMA)) {
            char** names = (char**)malloc(sizeof(char*) * 8);
            size_t name_count = 0;
            size_t name_cap = 8;
            names[name_count++] = name;
            while (match(parser, TOK_COMMA)) {
                consume(parser, TOK_IDENT, "Expect variable name in multi-variable assignment");
                if (name_count >= name_cap) {
                    name_cap *= 2;
                    names = (char**)realloc(names, sizeof(char*) * name_cap);
                }
                names[name_count++] = strdup(parser->previous.as.s_val);
            }

            if (match(parser, TOK_WALRUS) || match(parser, TOK_ASSIGN)) {
                TokenType op = parser->previous.type;
                AstNode* first_expr = parse_expression(parser);
                AstNode* expr = first_expr;
                if (match(parser, TOK_COMMA)) {
                    AstNode* tuple_node = ast_new(AST_TUPLE_LIT, line);
                    node_list_init(&tuple_node->as.collection.items);
                    node_list_add(&tuple_node->as.collection.items, first_expr);
                    do {
                        node_list_add(&tuple_node->as.collection.items, parse_expression(parser));
                    } while (match(parser, TOK_COMMA));
                    expr = tuple_node;
                }
                match(parser, TOK_SEMICOLON);
                AstNode* n = ast_new((op == TOK_WALRUS) ? AST_STMT_MULTI_VAR_DECL : AST_STMT_MULTI_ASSIGN, line);
                n->as.multi_assign.names = names;
                n->as.multi_assign.count = name_count;
                n->as.multi_assign.expr = expr;
                return n;
            } else {
                error_at(parser, &parser->current, "Expect ':=' or '=' after variable list");
                return NULL;
            }
        }

        if (match(parser, TOK_KW_L) || match(parser, TOK_KW_T) ||
            match(parser, TOK_KW_SL) || match(parser, TOK_KW_DICT) ||
            match(parser, TOK_KW_SET)) {
            TokenType coll_type = parser->previous.type;
            AstNode* init = NULL;
            if (match(parser, TOK_ASSIGN)) {
                init = parse_expression(parser);
            }
            match(parser, TOK_SEMICOLON);
            AstNode* n = ast_new(AST_STMT_VAR_DECL, line);
            n->as.var_decl.name = name;
            n->as.var_decl.type_tok = coll_type;
            n->as.var_decl.size_expr = NULL;
            n->as.var_decl.init_expr = init;
            n->as.var_decl.is_walrus = false;
            return n;
        }

        if (match(parser, TOK_KW_I) || match(parser, TOK_KW_D) ||
            match(parser, TOK_KW_B) || match(parser, TOK_KW_C) ||
            match(parser, TOK_KW_S)) {
            TokenType elem_type = parser->previous.type;
            AstNode* size_node = NULL;
            AstNode* init_node = NULL;
            if (match(parser, TOK_ASSIGN)) {
                init_node = parse_expression(parser);
            } else {
                size_node = parse_expression(parser);
            }
            match(parser, TOK_SEMICOLON);
            AstNode* n = ast_new(AST_STMT_VAR_DECL, line);
            n->as.var_decl.name = name;
            n->as.var_decl.type_tok = elem_type;
            n->as.var_decl.size_expr = size_node;
            n->as.var_decl.init_expr = init_node;
            n->as.var_decl.is_walrus = false;
            return n;
        }

        if (match(parser, TOK_WALRUS)) {
            AstNode* init = parse_expression(parser);
            match(parser, TOK_SEMICOLON);
            AstNode* n = ast_new(AST_STMT_VAR_DECL, line);
            n->as.var_decl.name = name;
            n->as.var_decl.type_tok = TOK_WALRUS;
            n->as.var_decl.init_expr = init;
            n->as.var_decl.is_walrus = true;
            return n;
        }

        if (match(parser, TOK_ASSIGN) || match(parser, TOK_PLUS_ASSIGN) ||
            match(parser, TOK_MINUS_ASSIGN) || match(parser, TOK_STAR_ASSIGN) ||
            match(parser, TOK_SLASH_ASSIGN)) {
            TokenType op = parser->previous.type;
            AstNode* val = parse_expression(parser);
            match(parser, TOK_SEMICOLON);
            AstNode* var = ast_new(AST_VARIABLE, line);
            var->as.variable.name = name;
            AstNode* assign = ast_new(AST_STMT_ASSIGN, line);
            assign->as.assign.target = var;
            assign->as.assign.op = op;
            assign->as.assign.value = val;
            return assign;
        }

        AstNode* expr_var = ast_new(AST_VARIABLE, line);
        expr_var->as.variable.name = name;

        AstNode* expr = expr_var;
        for (;;) {
            int pline = parser->current.line;
            if (match(parser, TOK_LPAREN)) {
                AstNode* call = ast_new(AST_CALL, pline);
                call->as.call.callee = expr;
                node_list_init(&call->as.call.args);
                if (!check(parser, TOK_RPAREN)) {
                    do {
                        node_list_add(&call->as.call.args, parse_call_arg(parser));
                    } while (match(parser, TOK_COMMA));
                }
                consume(parser, TOK_RPAREN, "Expect ')' after arguments");
                expr = call;
                continue;
            }
            if (match(parser, TOK_LBRACKET)) {
                AstNode* start = NULL;
                if (!check(parser, TOK_COLON)) {
                    start = parse_expression(parser);
                }
                if (match(parser, TOK_COLON)) {
                    AstNode* end = NULL;
                    if (!check(parser, TOK_RBRACKET)) end = parse_expression(parser);
                    consume(parser, TOK_RBRACKET, "Expect ']' after slice");
                    AstNode* slice = ast_new(AST_SLICE, pline);
                    slice->as.slice.target = expr;
                    slice->as.slice.start = start;
                    slice->as.slice.end = end;
                    expr = slice;
                } else {
                    consume(parser, TOK_RBRACKET, "Expect ']' after index");
                    AstNode* idx = ast_new(AST_INDEX_GET, pline);
                    idx->as.index.target = expr;
                    idx->as.index.index = start;
                    expr = idx;
                }
                continue;
            }
            if (match(parser, TOK_DOT)) {
                char* mname = NULL;
                if (match(parser, TOK_KW_T)) mname = strdup("T");
                else if (match(parser, TOK_IDENT)) mname = strdup(parser->previous.as.s_val);
                else { error_at(parser, &parser->current, "Expect member name"); break; }

                if (match(parser, TOK_LPAREN)) {
                    AstNode* mc = ast_new(AST_METHOD_CALL, pline);
                    mc->as.method_call.target = expr;
                    mc->as.method_call.method_name = mname;
                    node_list_init(&mc->as.method_call.args);
                    if (!check(parser, TOK_RPAREN)) {
                        do {
                            node_list_add(&mc->as.method_call.args, parse_call_arg(parser));
                        } while (match(parser, TOK_COMMA));
                    }
                    consume(parser, TOK_RPAREN, "Expect ')' after method arguments");
                    expr = mc;
                } else {
                    AstNode* prop = ast_new(AST_PROP_GET, pline);
                    prop->as.prop.target = expr;
                    prop->as.prop.prop_name = mname;
                    expr = prop;
                }
                continue;
            }
            break;
        }

        if (match(parser, TOK_ASSIGN)) {
            AstNode* val = parse_expression(parser);
            match(parser, TOK_SEMICOLON);
            if (expr->type == AST_INDEX_GET) {
                AstNode* set_idx = ast_new(AST_INDEX_SET, line);
                set_idx->as.index.target = expr->as.index.target;
                set_idx->as.index.index = expr->as.index.index;
                set_idx->as.index.value = val;
                return set_idx;
            } else if (expr->type == AST_PROP_GET) {
                AstNode* set_prop = ast_new(AST_PROP_SET, line);
                set_prop->as.prop.target = expr->as.prop.target;
                set_prop->as.prop.prop_name = expr->as.prop.prop_name;
                set_prop->as.prop.value = val;
                return set_prop;
            }
        }

        match(parser, TOK_SEMICOLON);
        AstNode* stmt = ast_new(AST_STMT_EXPR, line);
        stmt->as.expr_stmt.expr = expr;
        return stmt;
    }

    AstNode* expr = parse_expression(parser);
    match(parser, TOK_SEMICOLON);
    if (expr->type == AST_STMT_ASSIGN || expr->type == AST_INDEX_SET || expr->type == AST_PROP_SET) {
        return expr;
    }
    AstNode* stmt = ast_new(AST_STMT_EXPR, line);
    stmt->as.expr_stmt.expr = expr;
    return stmt;
}

AstNode* parse_program(Parser* parser) {
    AstNode* prog = ast_new(AST_PROGRAM, 1);
    node_list_init(&prog->as.block.statements);

    while (!check(parser, TOK_EOF)) {
        AstNode* stmt = parse_statement(parser);
        if (stmt) {
            node_list_add(&prog->as.block.statements, stmt);
        }
    }
    return prog;
}

