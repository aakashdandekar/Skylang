
#include "../include/sky_rust.h"
#include "../include/sky_platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

Value sky_mod_rust;

static int rust_file_counter = 0;

typedef struct {
    const char* src;
    size_t pos;
} RustJsonParser;

static void rust_skip_ws(RustJsonParser* p) {
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos])) p->pos++;
}

static Value rust_parse_val(RustJsonParser* p);

static Value rust_parse_str(RustJsonParser* p) {
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

static Value rust_parse_num(RustJsonParser* p) {
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

static Value rust_parse_list(RustJsonParser* p) {
    p->pos++;
    Value res = val_list();
    ObjList* l = as_list(res);
    rust_skip_ws(p);
    if (p->src[p->pos] == ']') { p->pos++; return res; }
    for (;;) {
        rust_skip_ws(p);
        list_push(l, rust_parse_val(p));
        rust_skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == ']') { p->pos++; break; }
        break;
    }
    return res;
}

static Value rust_parse_dict(RustJsonParser* p) {
    p->pos++;
    Value res = val_dict();
    ObjDict* d = as_dict(res);
    rust_skip_ws(p);
    if (p->src[p->pos] == '}') { p->pos++; return res; }
    for (;;) {
        rust_skip_ws(p);
        if (p->src[p->pos] != '"') break;
        Value k = rust_parse_str(p);
        rust_skip_ws(p);
        if (p->src[p->pos] == ':') p->pos++;
        rust_skip_ws(p);
        Value v = rust_parse_val(p);
        dict_set(d, k, v);
        rust_skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == '}') { p->pos++; break; }
        break;
    }
    return res;
}

static Value rust_parse_val(RustJsonParser* p) {
    rust_skip_ws(p);
    char c = p->src[p->pos];
    if (!c) return val_nil();
    if (c == '"') return rust_parse_str(p);
    if (c == '[') return rust_parse_list(p);
    if (c == '{') return rust_parse_dict(p);
    if (c == '-' || isdigit((unsigned char)c)) return rust_parse_num(p);
    if (strncmp(p->src + p->pos, "true", 4) == 0) { p->pos += 4; return val_bool(true); }
    if (strncmp(p->src + p->pos, "false", 5) == 0) { p->pos += 5; return val_bool(false); }
    if (strncmp(p->src + p->pos, "null", 4) == 0) { p->pos += 4; return val_nil(); }

    size_t start = p->pos;
    while (p->src[p->pos] && p->src[p->pos] != '\n' && p->src[p->pos] != '\r') p->pos++;
    size_t len = p->pos - start;
    char* buf = (char*)GC_MALLOC(len + 1);
    memcpy(buf, p->src + start, len);
    buf[len] = '\0';
    return val_string(buf);
}

static Value rust_parse_output(const char* output) {
    if (!output || !*output) return val_nil();
    RustJsonParser p = { .src = output, .pos = 0 };
    return rust_parse_val(&p);
}

static char* run_rust_cmd_capture(const char* cmd, int* exit_code) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        if (exit_code) *exit_code = -1;
        return strdup("");
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
    int status = pclose(pipe);
    if (exit_code) *exit_code = status;
    return buf;
}

Value sky_rust_load(const char* path_or_crate) {
    if (strstr(path_or_crate, ".so") != NULL || strstr(path_or_crate, ".dylib") != NULL ||
        strstr(path_or_crate, ".dll") != NULL || sky_access(path_or_crate, F_OK) == 0) {
        void* handle = sky_dlopen(path_or_crate);
        if (!handle) {
            sky_runtime_error("OSError", "Failed to open shared library '%s': %s", path_or_crate, sky_dlerror());
            return val_nil();
        }
        return val_foreign(FOREIGN_RUST, path_or_crate, handle, NULL);
    }
    return val_foreign(FOREIGN_RUST, path_or_crate, strdup(path_or_crate), NULL);
}

Value sky_rust_compile(const char* rust_code) {
    int fid = ++rust_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char rs_file[2048];
    char so_file[2048];
    snprintf(rs_file, sizeof(rs_file), "%s/sky_rust_%d_%d.rs", tmp_dir, (int)sky_getpid(), fid);
    snprintf(so_file, sizeof(so_file), "%s/sky_rust_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_SO_EXT);

    FILE* f = fopen(rs_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Rust source file");
        return val_nil();
    }

    fputs("#![allow(unused_imports, dead_code, non_snake_case)]\n", f);
    fputs("use std::collections::*;\n\n", f);
    fputs(rust_code, f);
    fclose(f);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "rustc --crate-type cdylib -O \"%s\" -o \"%s\" 2>&1", rs_file, so_file);
    int status = 0;
    char* err = run_rust_cmd_capture(cmd, &status);
    sky_unlink(rs_file);

    if (status != 0) {
        sky_unlink(so_file);
        sky_runtime_error("CompileError", "Rust inline compilation failed:\n%s", err);
        free(err);
        return val_nil();
    }
    free(err);

    void* handle = sky_dlopen(so_file);
    if (!handle) {
        sky_runtime_error("OSError", "dlopen failed on compiled Rust library: %s", sky_dlerror());
        return val_nil();
    }
    return val_foreign(FOREIGN_RUST, "compiled_rust", handle, NULL);
}

static Value rust_eval_internal(const char* expr) {
    char wrapper[4096];
    snprintf(wrapper, sizeof(wrapper),
             "#[no_mangle]\n"
             "pub extern \"C\" fn _sky_rust_eval_fn() -> f64 {\n"
             "    (%s) as f64\n"
             "}\n",
             expr);

    int fid = ++rust_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char rs_file[2048];
    char so_file[2048];
    snprintf(rs_file, sizeof(rs_file), "%s/sky_rust_eval_%d_%d.rs", tmp_dir, (int)sky_getpid(), fid);
    snprintf(so_file, sizeof(so_file), "%s/sky_rust_eval_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_SO_EXT);

    FILE* f = fopen(rs_file, "w");
    if (f) {
        fputs("#![allow(unused_imports, dead_code)]\n", f);
        fputs(wrapper, f);
        fclose(f);

        char cmd[8192];
        snprintf(cmd, sizeof(cmd), "rustc --crate-type cdylib -O \"%s\" -o \"%s\" 2>&1", rs_file, so_file);
        int status = 0;
        char* err = run_rust_cmd_capture(cmd, &status);
        sky_unlink(rs_file);
        if (status == 0) {
            free(err);
            void* handle = sky_dlopen(so_file);
            if (handle) {
                typedef double (*EvalFn)(void);
                EvalFn fn = (EvalFn)sky_dlsym(handle, "_sky_rust_eval_fn");
                if (fn) {
                    double d = fn();
                    sky_dlclose(handle);
                    sky_unlink(so_file);
                    return val_double(d);
                }
                sky_dlclose(handle);
            }
        }
        free(err);
        sky_unlink(so_file);
    }

    char bin_file[2048];
    snprintf(rs_file, sizeof(rs_file), "%s/sky_rust_runner_%d_%d.rs", tmp_dir, (int)sky_getpid(), fid);
    snprintf(bin_file, sizeof(bin_file), "%s/sky_rust_runner_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_EXE_EXT);

    f = fopen(rs_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Rust runner file");
        return val_nil();
    }

    fputs("#![allow(unused_imports, dead_code)]\n", f);
    fputs("use std::fmt::Write;\n\n", f);
    fputs("fn main() {\n", f);
    fprintf(f, "    let val = %s;\n", expr);
    fputs("    print!(\"{:?}\", val);\n", f);
    fputs("}\n", f);
    fclose(f);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "rustc -O \"%s\" -o \"%s\" 2>&1 && \"%s\" 2>&1", rs_file, bin_file, bin_file);
    int status = 0;
    char* out = run_rust_cmd_capture(cmd, &status);
    sky_unlink(rs_file);
    sky_unlink(bin_file);

    if (status != 0) {
        sky_runtime_error("RustError", "Rust evaluation failed:\n%s", out);
        free(out);
        return val_nil();
    }

    Value res = rust_parse_output(out);
    free(out);
    return res;
}

Value sky_rust_exec(const char* code) {
    int fid = ++rust_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char rs_file[2048];
    char bin_file[2048];
    snprintf(rs_file, sizeof(rs_file), "%s/sky_rust_exec_%d_%d.rs", tmp_dir, (int)sky_getpid(), fid);
    snprintf(bin_file, sizeof(bin_file), "%s/sky_rust_exec_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_EXE_EXT);

    FILE* f = fopen(rs_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Rust source file");
        return val_nil();
    }

    fputs("#![allow(unused_imports, dead_code)]\n", f);
    if (strstr(code, "fn main") == NULL) {
        fputs("fn main() {\n", f);
        fputs(code, f);
        fputs("\n}\n", f);
    } else {
        fputs(code, f);
    }
    fclose(f);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "rustc -O \"%s\" -o \"%s\" 2>&1 && \"%s\" 2>&1", rs_file, bin_file, bin_file);
    int status = 0;
    char* out = run_rust_cmd_capture(cmd, &status);
    sky_unlink(rs_file);
    sky_unlink(bin_file);

    if (status != 0) {
        sky_runtime_error("RustError", "Rust execution failed:\n%s", out);
        free(out);
        return val_nil();
    }

    Value res = val_string(out);
    free(out);
    return res;
}

typedef double (*RustDoubleFn0)(void);
typedef double (*RustDoubleFn1)(double);
typedef double (*RustDoubleFn2)(double, double);
typedef double (*RustDoubleFn3)(double, double, double);
typedef double (*RustDoubleFn4)(double, double, double, double);

typedef int64_t (*RustIntFn0)(void);
typedef int64_t (*RustIntFn1)(int64_t);
typedef int64_t (*RustIntFn2)(int64_t, int64_t);

static double rust_extract_double(Value v) {
    if (v.type == VAL_INT) return (double)v.as.i;
    if (v.type == VAL_DOUBLE) return v.as.d;
    return 0.0;
}

Value sky_rust_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f) return val_nil();

    if (f->extra == NULL && f->handle && strstr(f->name, "compiled_rust") != NULL) {
        void* sym = sky_dlsym(f->handle, name);
        if (!sym) {
            sky_runtime_error("AttributeError", "Symbol '%s' not found in Rust shared library: %s", name, sky_dlerror());
            return val_nil();
        }
        switch (argc) {
            case 0: return val_double(((RustDoubleFn0)sym)());
            case 1: return val_double(((RustDoubleFn1)sym)(rust_extract_double(argv[0])));
            case 2: return val_double(((RustDoubleFn2)sym)(rust_extract_double(argv[0]), rust_extract_double(argv[1])));
            case 3: return val_double(((RustDoubleFn3)sym)(rust_extract_double(argv[0]), rust_extract_double(argv[1]), rust_extract_double(argv[2])));
            case 4: return val_double(((RustDoubleFn4)sym)(rust_extract_double(argv[0]), rust_extract_double(argv[1]), rust_extract_double(argv[2]), rust_extract_double(argv[3])));
            default:
                sky_runtime_error("TypeError", "Functions with > 4 arguments not supported via dynamic Rust dispatcher");
                return val_nil();
        }
    }

    char call_expr[4096];
    char args_buf[2048] = "";
    for (int i = 0; i < argc; ++i) {
        char arg_str[256];
        if (argv[i].type == VAL_INT) {
            snprintf(arg_str, sizeof(arg_str), "%lld", (long long)argv[i].as.i);
        } else if (argv[i].type == VAL_DOUBLE) {
            snprintf(arg_str, sizeof(arg_str), "%f", argv[i].as.d);
        } else if (argv[i].type == VAL_BOOL) {
            snprintf(arg_str, sizeof(arg_str), "%s", argv[i].as.b ? "true" : "false");
        } else if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            snprintf(arg_str, sizeof(arg_str), "\"%s\"", ((ObjString*)argv[i].as.obj)->chars);
        } else {
            snprintf(arg_str, sizeof(arg_str), "()");
        }
        if (i > 0) strncat(args_buf, ", ", sizeof(args_buf) - strlen(args_buf) - 1);
        strncat(args_buf, arg_str, sizeof(args_buf) - strlen(args_buf) - 1);
    }

    const char* mod = f->name ? f->name : "std::cmp";
    snprintf(call_expr, sizeof(call_expr), "%s::%s(%s)", mod, name, args_buf);
    return rust_eval_internal(call_expr);
}

Value sky_rust_get_prop(ObjForeign* f, const char* name) {
    if (!f) return val_nil();

    if (f->name && (strcmp(f->name, "std::f64::consts") == 0 || strcmp(f->name, "math") == 0)) {
        if (strcmp(name, "PI") == 0 || strcmp(name, "pi") == 0) return val_double(3.14159265358979323846);
        if (strcmp(name, "E") == 0 || strcmp(name, "e") == 0) return val_double(2.71828182845904523536);
        if (strcmp(name, "SQRT_2") == 0 || strcmp(name, "sqrt2") == 0) return val_double(1.41421356237309504880);
        if (strcmp(name, "LN_2") == 0 || strcmp(name, "ln2") == 0) return val_double(0.693147180559945309417);
        if (strcmp(name, "TAU") == 0 || strcmp(name, "tau") == 0) return val_double(6.28318530717958647692);
    }

    if (f->handle && strstr(f->name, "compiled_rust") != NULL) {
        void* sym = sky_dlsym(f->handle, name);
        if (sym) return val_foreign(FOREIGN_RUST, name, f->handle, sym);
    }

    char prop_expr[512];
    snprintf(prop_expr, sizeof(prop_expr), "%s::%s", f->name ? f->name : "", name);
    return rust_eval_internal(prop_expr);
}

Value sky_rust_set_prop(ObjForeign* f, const char* name, Value val) {
    (void)f; (void)name; (void)val;
    return val_nil();
}

static Value rust_native_load(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    if (argc == 1) {
        if (argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
        return sky_rust_load(((ObjString*)argv[0].as.obj)->chars);
    }
    Value list = val_list();
    ObjList* l = as_list(list);
    for (int i = 0; i < argc; ++i) {
        if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            list_push(l, sky_rust_load(((ObjString*)argv[i].as.obj)->chars));
        }
    }
    return list;
}

static Value rust_native_compile(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_rust_compile(((ObjString*)argv[0].as.obj)->chars);
}

static Value rust_native_exec(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_rust_exec(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_rust_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"),    val_function("load",    rust_native_load,    -1));
    dict_set(d, val_string("compile"), val_function("compile", rust_native_compile,  1));
    dict_set(d, val_string("exec"),    val_function("exec",    rust_native_exec,     1));

    sky_mod_rust = mod;
    return mod;
}

