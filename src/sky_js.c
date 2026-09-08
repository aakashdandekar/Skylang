
#include "../include/sky_js.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gc.h>

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

static char* run_node_eval(const char* js_code) {
    char tmp_js[1024];
    snprintf(tmp_js, sizeof(tmp_js), "/tmp/sky_js_%d.js", getpid());
    FILE* f = fopen(tmp_js, "w");
    if (!f) return strdup("null");
    fputs(js_code, f);
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "node %s 2>/dev/null", tmp_js);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        unlink(tmp_js);
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
    unlink(tmp_js);

    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

Value sky_js_load(const char* package_name) {
    return val_foreign(FOREIGN_JS, package_name, strdup(package_name), NULL);
}

Value sky_js_exec(const char* code) {
    char* json = run_node_eval(code);
    free(json);
    return val_nil();
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

