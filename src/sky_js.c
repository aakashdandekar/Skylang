
#include "../include/skylang.h"
#include "../include/sky_js.h"
#include "../include/sky_platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value sky_mod_js;

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} JsonBuf;

static void jbuf_init(JsonBuf* b) {
    b->cap = 256;
    b->len = 0;
    b->data = (char*)GC_MALLOC(b->cap);
    b->data[0] = '\0';
}

static void jbuf_append(JsonBuf* b, const char* str, size_t n) {
    while (b->len + n + 1 >= b->cap) {
        b->cap *= 2;
        b->data = (char*)GC_REALLOC(b->data, b->cap);
    }
    memcpy(b->data + b->len, str, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void jbuf_puts(JsonBuf* b, const char* str) {
    jbuf_append(b, str, strlen(str));
}

static void value_to_json_rec(JsonBuf* b, Value v) {
    switch (v.type) {
        case VAL_NIL:
            jbuf_puts(b, "null");
            break;
        case VAL_INT: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%lld", (long long)v.as.i);
            jbuf_puts(b, tmp);
            break;
        }
        case VAL_DOUBLE: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%g", v.as.d);
            jbuf_puts(b, tmp);
            break;
        }
        case VAL_BOOL:
            jbuf_puts(b, v.as.b ? "true" : "false");
            break;
        case VAL_CHAR: {
            char tmp[4] = {'"', v.as.c, '"', '\0'};
            jbuf_puts(b, tmp);
            break;
        }
        case VAL_TYPE:
            jbuf_puts(b, "\"");
            jbuf_puts(b, sky_type_name(v.as.t));
            jbuf_puts(b, "\"");
            break;
        case VAL_OBJ: {
            if (!v.as.obj) {
                jbuf_puts(b, "null");
                break;
            }
            switch (v.as.obj->type) {
                case OBJ_STRING: {
                    ObjString* s = (ObjString*)v.as.obj;
                    jbuf_puts(b, "\"");
                    for (const char* p = s->chars ? s->chars : ""; *p; ++p) {
                        if (*p == '"') jbuf_puts(b, "\\\"");
                        else if (*p == '\\') jbuf_puts(b, "\\\\");
                        else if (*p == '\n') jbuf_puts(b, "\\n");
                        else if (*p == '\t') jbuf_puts(b, "\\t");
                        else jbuf_append(b, p, 1);
                    }
                    jbuf_puts(b, "\"");
                    break;
                }
                case OBJ_LIST: {
                    ObjList* l = (ObjList*)v.as.obj;
                    jbuf_puts(b, "[");
                    for (size_t i = 0; i < l->count; ++i) {
                        if (i > 0) jbuf_puts(b, ",");
                        value_to_json_rec(b, l->items[i]);
                    }
                    jbuf_puts(b, "]");
                    break;
                }
                case OBJ_ARRAY: {
                    ObjArray* a = (ObjArray*)v.as.obj;
                    jbuf_puts(b, "[");
                    for (size_t i = 0; i < a->capacity; ++i) {
                        if (i > 0) jbuf_puts(b, ",");
                        value_to_json_rec(b, a->items[i]);
                    }
                    jbuf_puts(b, "]");
                    break;
                }
                case OBJ_TUPLE: {
                    ObjTuple* t = (ObjTuple*)v.as.obj;
                    jbuf_puts(b, "[");
                    for (size_t i = 0; i < t->count; ++i) {
                        if (i > 0) jbuf_puts(b, ",");
                        value_to_json_rec(b, t->items[i]);
                    }
                    jbuf_puts(b, "]");
                    break;
                }
                case OBJ_DICT: {
                    ObjDict* d = (ObjDict*)v.as.obj;
                    jbuf_puts(b, "{");
                    size_t written = 0;
                    for (size_t i = 0; i < d->capacity; ++i) {
                        if (d->entries[i].occupied) {
                            if (written > 0) jbuf_puts(b, ",");
                            char* ks = val_to_string(d->entries[i].key);
                            jbuf_puts(b, "\"");
                            jbuf_puts(b, ks);
                            jbuf_puts(b, "\":");
                            free(ks);
                            value_to_json_rec(b, d->entries[i].value);
                            written++;
                        }
                    }
                    jbuf_puts(b, "}");
                    break;
                }
                default:
                    jbuf_puts(b, "null");
                    break;
            }
            break;
        }
    }
}

static char* sky_val_to_json(Value v) {
    JsonBuf b;
    jbuf_init(&b);
    value_to_json_rec(&b, v);
    return b.data;
}

typedef struct {
    const char* src;
    size_t pos;
} JsonParser;

static void skip_ws(JsonParser* p) {
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos])) p->pos++;
}

static Value json_parse_val(JsonParser* p);

static Value json_parse_str(JsonParser* p) {
    p->pos++;
    size_t start = p->pos;
    while (p->src[p->pos] && p->src[p->pos] != '"') {
        if (p->src[p->pos] == '\\' && p->src[p->pos + 1]) p->pos++;
        p->pos++;
    }
    size_t len = p->pos - start;
    char* buf = (char*)GC_MALLOC(len + 1);
    size_t out_len = 0;
    for (size_t i = start; i < p->pos; ++i) {
        if (p->src[i] == '\\' && i + 1 < p->pos) {
            i++;
            if (p->src[i] == 'n') buf[out_len++] = '\n';
            else if (p->src[i] == 't') buf[out_len++] = '\t';
            else if (p->src[i] == '"') buf[out_len++] = '"';
            else if (p->src[i] == '\\') buf[out_len++] = '\\';
            else buf[out_len++] = p->src[i];
        } else {
            buf[out_len++] = p->src[i];
        }
    }
    buf[out_len] = '\0';
    if (p->src[p->pos] == '"') p->pos++;
    return val_string(buf);
}

static Value json_parse_num(JsonParser* p) {
    size_t start = p->pos;
    bool is_float = false;
    if (p->src[p->pos] == '-') p->pos++;
    while (isdigit((unsigned char)p->src[p->pos]) || p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E' || p->src[p->pos] == '+') {
        if (p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E') is_float = true;
        p->pos++;
    }
    char buf[64];
    size_t len = p->pos - start;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, p->src + start, len);
    buf[len] = '\0';
    if (is_float) return val_double(atof(buf));
    return val_int(atoll(buf));
}

static Value json_parse_list(JsonParser* p) {
    p->pos++;
    Value res = val_list();
    ObjList* l = as_list(res);
    skip_ws(p);
    if (p->src[p->pos] == ']') { p->pos++; return res; }
    for (;;) {
        skip_ws(p);
        list_push(l, json_parse_val(p));
        skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == ']') { p->pos++; break; }
        break;
    }
    return res;
}

static Value json_parse_dict(JsonParser* p) {
    p->pos++;
    Value res = val_dict();
    ObjDict* d = as_dict(res);
    skip_ws(p);
    if (p->src[p->pos] == '}') { p->pos++; return res; }
    for (;;) {
        skip_ws(p);
        if (p->src[p->pos] != '"') break;
        Value k = json_parse_str(p);
        skip_ws(p);
        if (p->src[p->pos] == ':') p->pos++;
        skip_ws(p);
        Value v = json_parse_val(p);
        dict_set(d, k, v);
        skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == '}') { p->pos++; break; }
        break;
    }
    return res;
}

static Value json_parse_val(JsonParser* p) {
    skip_ws(p);
    char c = p->src[p->pos];
    if (c == '"') return json_parse_str(p);
    if (c == '[') return json_parse_list(p);
    if (c == '{') return json_parse_dict(p);
    if (isdigit((unsigned char)c) || c == '-') return json_parse_num(p);
    if (strncmp(p->src + p->pos, "true", 4) == 0) { p->pos += 4; return val_bool(true); }
    if (strncmp(p->src + p->pos, "false", 5) == 0) { p->pos += 5; return val_bool(false); }
    if (strncmp(p->src + p->pos, "null", 4) == 0) { p->pos += 4; return val_nil(); }
    p->pos++;
    return val_nil();
}

static Value json_to_sky_val(const char* json) {
    if (!json || !*json) return val_nil();
    JsonParser p;
    p.src = json;
    p.pos = 0;
    return json_parse_val(&p);
}

void foreign_symtable_init(ForeignSymbolTable* table) {
    table->items = NULL;
    table->count = 0;
    table->capacity = 0;
}

void foreign_symtable_add(ForeignSymbolTable* table, const char* name, const char* lang, ForeignDeclKind kind, const char* file, int line) {
    if (!name || !*name) return;
    for (size_t i = 0; i < table->count; ++i) {
        if (strcmp(table->items[i].name, name) == 0) return;
    }
    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        table->items = (ForeignSymbol*)realloc(table->items, sizeof(ForeignSymbol) * table->capacity);
    }
    ForeignSymbol* s = &table->items[table->count++];
    s->name = strdup(name);
    s->lang = lang;
    s->kind = kind;
    s->file = file ? file : "<unknown>";
    s->line = line;
}

void foreign_symtable_free(ForeignSymbolTable* table) {
    if (table->items) {
        for (size_t i = 0; i < table->count; ++i) {
            free(table->items[i].name);
        }
        free(table->items);
        table->items = NULL;
    }
    table->count = 0;
    table->capacity = 0;
}

static bool is_js_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '$';
}

static bool is_js_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '$';
}

static bool is_js_reserved_keyword(const char* id) {
    static const char* kws[] = {
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "break", "continue", "return", "try", "catch", "finally", "throw",
        "new", "delete", "typeof", "instanceof", "void", "in", "of",
        "this", "super", "class", "extends", "export", "import", "from",
        "as", "yield", "await", "async", "true", "false", "null", "undefined",
        "console", "require", "process", "global", "globalThis", "window",
        "document", "Math", "JSON", "Object", "Array", "String", "Number",
        "Boolean", "Function", "Symbol", "BigInt", "Promise", "RegExp", "Map", "Set"
    };
    for (size_t i = 0; i < sizeof(kws)/sizeof(kws[0]); ++i) {
        if (strcmp(id, kws[i]) == 0) return true;
    }
    return false;
}

void sky_extract_js_declarations(const char* code, const char* file, int base_line, ForeignSymbolTable* table) {
    if (!code) return;
    const char* p = code;
    int cur_line = base_line;
    int brace_depth = 0;

    while (*p) {
        if (*p == '\n') { cur_line++; p++; continue; }
        if (isspace((unsigned char)*p)) { p++; continue; }

        if (p[0] == '/' && p[1] == '/') {
            p += 2;
            while (*p && *p != '\n') p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                if (*p == '\n') cur_line++;
                p++;
            }
            if (*p) p += 2;
            continue;
        }
        if (*p == '"' || *p == '\'' || *p == '`') {
            char q = *p++;
            while (*p && *p != q) {
                if (*p == '\\' && *(p + 1)) {
                    if (*p == '\n') cur_line++;
                    p += 2;
                } else {
                    if (*p == '\n') cur_line++;
                    p++;
                }
            }
            if (*p == q) p++;
            continue;
        }

        if (*p == '{') { brace_depth++; p++; continue; }
        if (*p == '}') { if (brace_depth > 0) brace_depth--; p++; continue; }

        if (is_js_ident_start(*p)) {
            const char* start = p;
            while (is_js_ident_char(*p)) p++;
            size_t id_len = (size_t)(p - start);
            char ident[128];
            if (id_len >= sizeof(ident)) id_len = sizeof(ident) - 1;
            memcpy(ident, start, id_len);
            ident[id_len] = '\0';

            ForeignDeclKind kind = FOREIGN_DECL_ASSIGN;
            bool is_decl = false;

            if (strcmp(ident, "var") == 0) {
                kind = FOREIGN_DECL_VAR;
                is_decl = true;
            } else if (strcmp(ident, "let") == 0) {
                kind = FOREIGN_DECL_LET;
                is_decl = true;
            } else if (strcmp(ident, "const") == 0) {
                kind = FOREIGN_DECL_CONST;
                is_decl = true;
            } else if (strcmp(ident, "function") == 0) {
                kind = FOREIGN_DECL_FUNCTION;
                while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                if (is_js_ident_start(*p)) {
                    const char* fn_start = p;
                    while (is_js_ident_char(*p)) p++;
                    size_t fn_len = (size_t)(p - fn_start);
                    char fn_name[128];
                    if (fn_len >= sizeof(fn_name)) fn_len = sizeof(fn_name) - 1;
                    memcpy(fn_name, fn_start, fn_len);
                    fn_name[fn_len] = '\0';
                    if (!is_js_reserved_keyword(fn_name)) {
                        foreign_symtable_add(table, fn_name, "js", kind, file, cur_line);
                    }
                }
                continue;
            } else if (strcmp(ident, "class") == 0) {
                kind = FOREIGN_DECL_CLASS;
                while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                if (is_js_ident_start(*p)) {
                    const char* cl_start = p;
                    while (is_js_ident_char(*p)) p++;
                    size_t cl_len = (size_t)(p - cl_start);
                    char cl_name[128];
                    if (cl_len >= sizeof(cl_name)) cl_len = sizeof(cl_name) - 1;
                    memcpy(cl_name, cl_start, cl_len);
                    cl_name[cl_len] = '\0';
                    if (!is_js_reserved_keyword(cl_name)) {
                        foreign_symtable_add(table, cl_name, "js", kind, file, cur_line);
                    }
                }
                continue;
            }

            if (is_decl) {
                while (*p) {
                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (!is_js_ident_start(*p)) break;
                    const char* var_start = p;
                    while (is_js_ident_char(*p)) p++;
                    size_t var_len = (size_t)(p - var_start);
                    char var_name[128];
                    if (var_len >= sizeof(var_name)) var_len = sizeof(var_name) - 1;
                    memcpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';

                    if (!is_js_reserved_keyword(var_name)) {
                        foreign_symtable_add(table, var_name, "js", kind, file, cur_line);
                    }

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (*p == '=') {
                        p++;
                        int paren = 0, bracket = 0, brace = 0;
                        while (*p) {
                            if (*p == '(') paren++;
                            else if (*p == ')') { if (paren > 0) paren--; }
                            else if (*p == '[') bracket++;
                            else if (*p == ']') { if (bracket > 0) bracket--; }
                            else if (*p == '{') brace++;
                            else if (*p == '}') { if (brace > 0) brace--; }
                            else if (*p == '"' || *p == '\'' || *p == '`') {
                                char q = *p++;
                                while (*p && *p != q) {
                                    if (*p == '\\' && *(p+1)) p += 2;
                                    else p++;
                                }
                                if (*p == q) p++;
                                continue;
                            } else if (paren == 0 && bracket == 0 && brace == 0) {
                                if (*p == ',' || *p == ';' || *p == '\n') break;
                            }
                            if (*p == '\n') cur_line++;
                            p++;
                        }
                    }
                    if (*p == ',') {
                        p++;
                        continue;
                    }
                    if (*p == ';' || *p == '\n') {
                        if (*p == '\n') cur_line++;
                        p++;
                        break;
                    }
                    break;
                }
                continue;
            }

            if (brace_depth == 0 && !is_js_reserved_keyword(ident)) {
                const char* save_p = p;
                while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                if (*p == '=' && *(p+1) != '=') {
                    foreign_symtable_add(table, ident, "js", FOREIGN_DECL_ASSIGN, file, cur_line);
                }
                p = save_p;
            }
            continue;
        }

        p++;
    }
}

static int js_file_counter = 0;

static char* run_node_eval(const char* js_code) {
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char tmp_js[2048];
    snprintf(tmp_js, sizeof(tmp_js), "%s/sky_js_%d_%d.js", tmp_dir, (int)sky_getpid(), ++js_file_counter);
    FILE* f = fopen(tmp_js, "w");
    if (!f) return strdup("null");
    fputs(js_code, f);
    fclose(f);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "node \"%s\" 2>%s", tmp_js, SKY_DEV_NULL);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        sky_unlink(tmp_js);
        return strdup("null");
    }

    size_t cap = 512, len = 0;
    char* buf = (char*)malloc(cap);
    char chunk[256];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        size_t clen = strlen(chunk);
        while (len + clen + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf + len, chunk, clen);
        len += clen;
    }
    buf[len] = '\0';
    pclose(pipe);
    sky_unlink(tmp_js);

    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

Value sky_js_load(const char* package_name) {
    return val_foreign(FOREIGN_JS, package_name, strdup(package_name), NULL);
}

Value sky_js_exec(const char* code) {
    if (!code || !*code) return val_dict();

    ForeignSymbolTable syms;
    foreign_symtable_init(&syms);
    sky_extract_js_declarations(code, "<js>", 1, &syms);

    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char tmp_js[2048];
    char tmp_vars[2048];
    int fid = ++js_file_counter;
    snprintf(tmp_js, sizeof(tmp_js), "%s/sky_js_exec_%d_%d.js", tmp_dir, (int)sky_getpid(), fid);
    snprintf(tmp_vars, sizeof(tmp_vars), "%s/sky_js_vars_%d_%d.json", tmp_dir, (int)sky_getpid(), fid);

    FILE* f = fopen(tmp_js, "w");
    if (!f) {
        foreign_symtable_free(&syms);
        return val_dict();
    }

    char js_vars_path[2048];
    snprintf(js_vars_path, sizeof(js_vars_path), "%s", tmp_vars);
    for (char* p = js_vars_path; *p; ++p) {
        if (*p == '\\') *p = '/';
    }

    fprintf(f, "const fs = require('fs');\n");
    fprintf(f, "(async () => {\n");
    fprintf(f, "    try {\n");
    fprintf(f, "%s\n\n", code);
    fprintf(f, "        const __sky_exports = {};\n");
    for (size_t i = 0; i < syms.count; ++i) {
        const char* sname = syms.items[i].name;
        fprintf(f, "        try { if (typeof %s !== 'undefined') __sky_exports['%s'] = %s; } catch(e) {}\n", sname, sname, sname);
    }
    fprintf(f, "        fs.writeFileSync('%s', JSON.stringify(__sky_exports));\n", js_vars_path);
    fprintf(f, "    } catch(__err) {\n");
    fprintf(f, "        console.error(__err);\n");
    fprintf(f, "        try { fs.writeFileSync('%s', JSON.stringify({ '__sky_error__': String(__err && __err.stack ? __err.stack : __err) })); } catch(e) {}\n", js_vars_path);
    fprintf(f, "    }\n");
    fprintf(f, "})();\n");
    fclose(f);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "node \"%s\"", tmp_js);
    int status = system(cmd);
    (void)status;

    Value result_dict = val_dict();

    FILE* vf = fopen(tmp_vars, "rb");
    if (vf) {
        fseek(vf, 0, SEEK_END);
        long sz = ftell(vf);
        fseek(vf, 0, SEEK_SET);
        if (sz > 0) {
            char* json_content = (char*)malloc(sz + 1);
            if (json_content) {
                size_t rd = fread(json_content, 1, sz, vf);
                json_content[rd] = '\0';
                Value parsed = json_to_sky_val(json_content);
                free(json_content);
                if (parsed.type == VAL_OBJ && parsed.as.obj->type == OBJ_DICT) {
                    result_dict = parsed;
                }
            }
        }
        fclose(vf);
    }

    sky_unlink(tmp_js);
    sky_unlink(tmp_vars);
    foreign_symtable_free(&syms);

    return result_dict;
}

Value sky_js_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f || !f->name) return val_nil();

    JsonBuf args_buf;
    jbuf_init(&args_buf);
    jbuf_puts(&args_buf, "[");
    for (int i = 0; i < argc; ++i) {
        if (i > 0) jbuf_puts(&args_buf, ",");
        value_to_json_rec(&args_buf, argv[i]);
    }
    jbuf_puts(&args_buf, "]");

    char script[16384];
    snprintf(script, sizeof(script),
             "try {\n"
             "    let target;\n"
             "    try { target = require('%s'); } catch(e) { target = globalThis['%s'] || eval('%s'); }\n"
             "    const fn = target['%s'] || target;\n"
             "    const args = %s;\n"
             "    const res = typeof fn === 'function' ? fn.apply(target, args) : fn;\n"
             "    console.log(JSON.stringify(res !== undefined ? res : null));\n"
             "} catch(e) {\n"
             "    console.error(e.message);\n"
             "    console.log('null');\n"
             "}\n",
             f->name, f->name, f->name, name, args_buf.data);

    char* json = run_node_eval(script);
    Value res = json_to_sky_val(json);
    free(json);
    return res;
}

Value sky_js_get_prop(ObjForeign* f, const char* name) {
    if (!f || !f->name) return val_nil();
    char script[4096];
    snprintf(script, sizeof(script),
             "try {\n"
             "    let target;\n"
             "    try { target = require('%s'); } catch(e) { target = globalThis['%s'] || eval('%s'); }\n"
             "    const res = target['%s'];\n"
             "    console.log(JSON.stringify(res !== undefined ? res : null));\n"
             "} catch(e) { console.log('null'); }\n",
             f->name, f->name, f->name, name);

    char* json = run_node_eval(script);
    Value res = json_to_sky_val(json);
    free(json);
    return res;
}

Value sky_js_set_prop(ObjForeign* f, const char* name, Value val) {
    if (!f || !f->name) return val_nil();
    char* json_val = sky_val_to_json(val);
    char script[4096];
    snprintf(script, sizeof(script),
             "try {\n"
             "    let target;\n"
             "    try { target = require('%s'); } catch(e) { target = globalThis['%s'] || eval('%s'); }\n"
             "    target['%s'] = %s;\n"
             "} catch(e) {}\n",
             f->name, f->name, f->name, name, json_val);
    char* res = run_node_eval(script);
    free(res);
    return val;
}

static Value js_native_load(int argc, Value* argv) {
    if (argc == 0) return val_nil();
    if (argc == 1) {
        if (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) {
            return sky_js_load(((ObjString*)argv[0].as.obj)->chars);
        }
        return val_nil();
    }
    Value list = val_list();
    ObjList* l = as_list(list);
    for (int i = 0; i < argc; ++i) {
        if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            list_push(l, sky_js_load(((ObjString*)argv[i].as.obj)->chars));
        }
    }
    return list;
}

static Value js_native_exec(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_js_exec(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_js_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"), val_function("load", js_native_load, -1));
    dict_set(d, val_string("exec"), val_function("exec", js_native_exec,  1));

    sky_mod_js = mod;
    return mod;
}

