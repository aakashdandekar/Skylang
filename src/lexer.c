#include "../include/skylang.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->col = 1;
}

static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    char c = *lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    advance(lexer);
    return true;
}

static Token make_token(Lexer* lexer, TokenType type, const char* start, int line, int col) {
    Token token;
    token.type = type;
    token.start = start;
    token.length = (size_t)(lexer->current - start);
    token.line = line;
    token.col = col;
    token.as.s_val = NULL;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOK_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.col = lexer->col;
    token.as.s_val = (char*)message;
    return token;
}

static bool last_token_can_end_expr = false;

static bool is_floor_div_syntax(const char* p) {
    if (p[0] != '/' || p[1] != '/') return false;
    p += 2;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '\n' || *p == '\r') return false;

    if (isupper((unsigned char)*p)) return false;

    if (isdigit((unsigned char)*p) || *p == '(' || *p == '-' || *p == '+') return true;

    const char* start_w = p;
    while (isalnum((unsigned char)*p) || *p == '_') p++;
    size_t wlen = (size_t)(p - start_w);
    if (wlen == 7 && strncmp(start_w, "comment", 7) == 0) return false;
    if (wlen == 4 && strncmp(start_w, "todo", 4) == 0) return false;
    if (wlen == 4 && strncmp(start_w, "note", 4) == 0) return false;

    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '\n' || *p == '\r' || *p == ';' || *p == ')' || *p == '}') {
        return true;
    }

    if (isalpha((unsigned char)*p) || *p == '_') return false;
    return true;
}

static void skip_whitespace_and_comments(Lexer* lexer, bool can_be_floordiv) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                advance(lexer);
                can_be_floordiv = false;
                last_token_can_end_expr = false;
                break;
            case '#':

                while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {

                    if (can_be_floordiv && is_floor_div_syntax(lexer->current)) {
                        return;
                    }

                    advance(lexer);
                    advance(lexer);
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                } else if (peek_next(lexer) == '*') {

                    advance(lexer);
                    advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token string_lit(Lexer* lexer, const char* start, int line, int col) {
    size_t cap = 32;
    size_t len = 0;
    char* buf = (char*)malloc(cap);

    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        char c = advance(lexer);
        if (c == '\\') {
            char esc = advance(lexer);
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '0': c = '\0'; break;
                default: c = esc; break;
            }
        }
        if (len + 1 >= cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        buf[len++] = c;
    }

    if (is_at_end(lexer)) {
        free(buf);
        return error_token(lexer, "Unterminated string literal");
    }

    advance(lexer);
    buf[len] = '\0';

    Token token = make_token(lexer, TOK_STRING_LIT, start, line, col);
    token.as.s_val = buf;
    return token;
}

static Token char_lit(Lexer* lexer, const char* start, int line, int col) {
    size_t cap = 16;
    size_t len = 0;
    char* buf = (char*)malloc(cap);

    while (peek(lexer) != '\'' && !is_at_end(lexer)) {
        char c = advance(lexer);
        if (c == '\\') {
            char esc = advance(lexer);
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '\'': c = '\''; break;
                case '0': c = '\0'; break;
                default: c = esc; break;
            }
        }
        if (len + 1 >= cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        buf[len++] = c;
    }

    if (is_at_end(lexer)) {
        free(buf);
        return error_token(lexer, "Unterminated character or string literal");
    }

    advance(lexer);
    buf[len] = '\0';

    if (len == 1) {
        Token token = make_token(lexer, TOK_CHAR_LIT, start, line, col);
        token.as.c_val = buf[0];
        free(buf);
        return token;
    } else {
        Token token = make_token(lexer, TOK_STRING_LIT, start, line, col);
        token.as.s_val = buf;
        return token;
    }
}

static Token fstring_lit(Lexer* lexer, char quote, const char* start, int line, int col) {
    size_t cap = 64;
    size_t len = 0;
    char* buf = (char*)malloc(cap);

    while (peek(lexer) != quote && !is_at_end(lexer)) {
        if (peek(lexer) == '\\') {
            char c1 = advance(lexer);
            if (len + 2 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = c1;
            if (!is_at_end(lexer)) {
                char c2 = advance(lexer);
                buf[len++] = c2;
            }
            continue;
        }

        if (peek(lexer) == '{') {
            if (peek_next(lexer) == '{') {
                if (len + 2 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
                buf[len++] = advance(lexer);
                buf[len++] = advance(lexer);
                continue;
            }

            if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = advance(lexer);

            int depth = 1;
            char in_quote = 0;
            while (depth > 0 && !is_at_end(lexer)) {
                if (peek(lexer) == '\\') {
                    char c1 = advance(lexer);
                    if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
                    buf[len++] = c1;
                    if (!is_at_end(lexer)) {
                        char esc = advance(lexer);
                        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
                        buf[len++] = esc;
                        if (in_quote == 0) {
                            if (esc == '"' || esc == '\'') {
                                in_quote = esc;
                            }
                        } else if (in_quote == esc) {
                            in_quote = 0;
                        }
                    }
                    continue;
                }

                char ch = advance(lexer);
                if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
                buf[len++] = ch;

                if (in_quote) {
                    if (ch == in_quote) {
                        in_quote = 0;
                    }
                } else {
                    if (ch == '"' || ch == '\'') {
                        in_quote = ch;
                    } else if (ch == '{') {
                        depth++;
                    } else if (ch == '}') {
                        depth--;
                    }
                }
            }
            continue;
        }

        char c = advance(lexer);
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = c;
    }

    if (is_at_end(lexer)) {
        free(buf);
        return error_token(lexer, "Unterminated f-string literal");
    }

    advance(lexer);
    buf[len] = '\0';

    Token token = make_token(lexer, TOK_FSTRING_LIT, start, line, col);
    token.as.s_val = buf;
    return token;
}

static Token number_lit(Lexer* lexer, const char* start, int line, int col) {
    while (isdigit(peek(lexer))) advance(lexer);

    bool is_double = false;
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        is_double = true;
        advance(lexer);
        while (isdigit(peek(lexer))) advance(lexer);
    }

    Token token;
    if (is_double) {
        token = make_token(lexer, TOK_DOUBLE_LIT, start, line, col);
        token.as.d_val = strtod(start, NULL);
    } else {
        token = make_token(lexer, TOK_INT_LIT, start, line, col);
        token.as.i_val = strtoll(start, NULL, 10);
    }
    return token;
}

static TokenType check_keyword(const char* start, size_t length) {

    if (length == 1) {
        switch (start[0]) {
            case 'I': return TOK_KW_I;
            case 'D': return TOK_KW_D;
            case 'B': return TOK_KW_B;
            case 'C': return TOK_KW_C;
            case 'S': return TOK_KW_S;
            case 'L': return TOK_KW_L;
            case 'T': return TOK_KW_T;
            case 'f': return TOK_KW_F;
        }
    }
    if (length == 2) {
        if (strncmp(start, "as", 2) == 0) return TOK_KW_AS;
        if (strncmp(start, "SL", 2) == 0) return TOK_KW_SL;
        if (strncmp(start, "if", 2) == 0) return TOK_KW_IF;
        if (strncmp(start, "in", 2) == 0) return TOK_KW_IN;
        if (strncmp(start, "or", 2) == 0) return TOK_OR;
    }
    if (length == 3) {
        if (strncmp(start, "SET", 3) == 0) return TOK_KW_SET;
        if (strncmp(start, "nil", 3) == 0) return TOK_KW_NIL;
        if (strncmp(start, "new", 3) == 0) return TOK_KW_NEW;
        if (strncmp(start, "and", 3) == 0) return TOK_AND;
        if (strncmp(start, "for", 3) == 0) return TOK_KW_FOR;
    }
    if (length == 4) {
        if (strncmp(start, "from", 4) == 0) return TOK_KW_FROM;
        if (strncmp(start, "DICT", 4) == 0) return TOK_KW_DICT;
        if (strncmp(start, "init", 4) == 0) return TOK_KW_INIT;
        if (strncmp(start, "this", 4) == 0) return TOK_KW_THIS;
        if (strncmp(start, "else", 4) == 0) return TOK_KW_ELSE;
        if (strncmp(start, "elif", 4) == 0) return TOK_KW_ELIF;
        if (strncmp(start, "true", 4) == 0) return TOK_KW_TRUE;
        if (strncmp(start, "none", 4) == 0 || strncmp(start, "None", 4) == 0) return TOK_KW_NIL;
    }
    if (length == 5) {
        if (strncmp(start, "takes", 5) == 0) return TOK_KW_TAKES;
        if (strncmp(start, "class", 5) == 0) return TOK_KW_CLASS;
        if (strncmp(start, "while", 5) == 0) return TOK_KW_WHILE;
        if (strncmp(start, "break", 5) == 0) return TOK_KW_BREAK;
        if (strncmp(start, "false", 5) == 0) return TOK_KW_FALSE;
    }
    if (length == 6) {
        if (strncmp(start, "return", 6) == 0) return TOK_KW_RETURN;
        if (strncmp(start, "extern", 6) == 0) return TOK_KW_EXTERN;
        if (strncmp(start, "import", 6) == 0) return TOK_KW_IMPORT;
    }
    if (length == 7) {
        if (strncmp(start, "cimport", 7) == 0) return TOK_KW_CIMPORT;
    }
    if (length == 8) {
        if (strncmp(start, "continue", 8) == 0) return TOK_KW_CONTINUE;
    }
    return TOK_IDENT;
}

static Token identifier(Lexer* lexer, const char* start, int line, int col) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    size_t len = (size_t)(lexer->current - start);
    TokenType type = check_keyword(start, len);
    Token token = make_token(lexer, type, start, line, col);
    if (type == TOK_IDENT) {
        token.as.s_val = (char*)malloc(len + 1);
        memcpy(token.as.s_val, start, len);
        token.as.s_val[len] = '\0';
    }
    return token;
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace_and_comments(lexer, last_token_can_end_expr);

    int start_line = lexer->line;
    int start_col = lexer->col;
    const char* start = lexer->current;

    if (is_at_end(lexer)) {
        last_token_can_end_expr = false;
        return make_token(lexer, TOK_EOF, start, start_line, start_col);
    }

    char c = advance(lexer);

    if (isdigit(c)) {
        Token t = number_lit(lexer, start, start_line, start_col);
        last_token_can_end_expr = true;
        return t;
    }

    if ((c == 'f' || c == 'F') && (peek(lexer) == '"' || peek(lexer) == '\'')) {
        char quote = advance(lexer);
        Token tok = fstring_lit(lexer, quote, start, start_line, start_col);
        last_token_can_end_expr = true;
        return tok;
    }

    if (isalpha(c) || c == '_') {
        Token t = identifier(lexer, start, start_line, start_col);
        last_token_can_end_expr = (t.type == TOK_IDENT || t.type == TOK_KW_TRUE || t.type == TOK_KW_FALSE || t.type == TOK_KW_NIL || t.type == TOK_KW_THIS);
        return t;
    }

    Token tok;
    switch (c) {
        case '(': tok = make_token(lexer, TOK_LPAREN, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case ')': tok = make_token(lexer, TOK_RPAREN, start, start_line, start_col); last_token_can_end_expr = true; return tok;
        case '{': tok = make_token(lexer, TOK_LBRACE, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case '}': tok = make_token(lexer, TOK_RBRACE, start, start_line, start_col); last_token_can_end_expr = true; return tok;
        case '[': tok = make_token(lexer, TOK_LBRACKET, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case ']': tok = make_token(lexer, TOK_RBRACKET, start, start_line, start_col); last_token_can_end_expr = true; return tok;
        case '.': tok = make_token(lexer, TOK_DOT, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case ',': tok = make_token(lexer, TOK_COMMA, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case ';': tok = make_token(lexer, TOK_SEMICOLON, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case '^': tok = make_token(lexer, TOK_CARET, start, start_line, start_col); last_token_can_end_expr = false; return tok;
        case '%': tok = make_token(lexer, TOK_PERCENT, start, start_line, start_col); last_token_can_end_expr = false; return tok;

        case ':':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_WALRUS, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_COLON, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '+':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_PLUS_ASSIGN, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_PLUS, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '-':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_MINUS_ASSIGN, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_MINUS, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '*':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_STAR_ASSIGN, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_STAR, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '/':
            if (match(lexer, '/')) {
                tok = make_token(lexer, TOK_FLOORDIV, start, start_line, start_col);
            } else if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_SLASH_ASSIGN, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_SLASH, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '=':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_EQ, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_ASSIGN, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '!':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_NEQ, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_BANG, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '<':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_LTE, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_LT, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '>':
            if (match(lexer, '=')) {
                tok = make_token(lexer, TOK_GTE, start, start_line, start_col);
            } else {
                tok = make_token(lexer, TOK_GT, start, start_line, start_col);
            }
            last_token_can_end_expr = false;
            return tok;

        case '&':
            if (match(lexer, '&')) {
                tok = make_token(lexer, TOK_AND, start, start_line, start_col);
                last_token_can_end_expr = false;
                return tok;
            }
            break;

        case '|':
            if (match(lexer, '|')) {
                tok = make_token(lexer, TOK_OR, start, start_line, start_col);
                last_token_can_end_expr = false;
                return tok;
            }
            break;

        case '"':
            tok = string_lit(lexer, start, start_line, start_col);
            last_token_can_end_expr = true;
            return tok;

        case '\'':
            tok = char_lit(lexer, start, start_line, start_col);
            last_token_can_end_expr = true;
            return tok;
    }

    return error_token(lexer, "Unexpected character");
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        case TOK_INT_LIT: return "INT_LIT";
        case TOK_DOUBLE_LIT: return "DOUBLE_LIT";
        case TOK_STRING_LIT: return "STRING_LIT";
        case TOK_CHAR_LIT: return "CHAR_LIT";
        case TOK_FSTRING_LIT: return "FSTRING_LIT";
        case TOK_IDENT: return "IDENT";
        case TOK_KW_I: return "I";
        case TOK_KW_D: return "D";
        case TOK_KW_B: return "B";
        case TOK_KW_C: return "C";
        case TOK_KW_S: return "S";
        case TOK_KW_L: return "L";
        case TOK_KW_T: return "T";
        case TOK_KW_SL: return "SL";
        case TOK_KW_DICT: return "DICT";
        case TOK_KW_SET: return "SET";
        case TOK_KW_F: return "f";
        case TOK_KW_RETURN: return "return";
        case TOK_KW_CLASS: return "class";
        case TOK_KW_INIT: return "init";
        case TOK_KW_THIS: return "this";
        case TOK_KW_TAKES: return "takes";
        case TOK_KW_IF: return "if";
        case TOK_KW_ELSE: return "else";
        case TOK_KW_ELIF: return "elif";
        case TOK_KW_WHILE: return "while";
        case TOK_KW_FOR: return "for";
        case TOK_KW_IN: return "in";
        case TOK_KW_BREAK: return "break";
        case TOK_KW_CONTINUE: return "continue";
        case TOK_KW_TRUE: return "true";
        case TOK_KW_FALSE: return "false";
        case TOK_KW_NIL: return "none";
        case TOK_KW_NEW: return "new";
        case TOK_KW_EXTERN: return "extern";
        case TOK_KW_CIMPORT: return "cimport";
        case TOK_KW_IMPORT: return "import";
        case TOK_KW_FROM: return "from";
        case TOK_KW_AS: return "as";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_FLOORDIV: return "//";
        case TOK_CARET: return "^";
        case TOK_PERCENT: return "%";
        case TOK_ASSIGN: return "=";
        case TOK_WALRUS: return ":=";
        case TOK_PLUS_ASSIGN: return "+=";
        case TOK_MINUS_ASSIGN: return "-=";
        case TOK_STAR_ASSIGN: return "*=";
        case TOK_SLASH_ASSIGN: return "/=";
        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_LT: return "<";
        case TOK_LTE: return "<=";
        case TOK_GT: return ">";
        case TOK_GTE: return ">=";
        case TOK_BANG: return "!";
        case TOK_AND: return "and";
        case TOK_OR: return "or";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_DOT: return ".";
        case TOK_COMMA: return ",";
        case TOK_COLON: return ":";
        case TOK_SEMICOLON: return ";";
    }
    return "UNKNOWN";
}

