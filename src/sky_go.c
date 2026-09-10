
#include "../include/sky_go.h"
#include "../include/sky_platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

Value sky_mod_golang;

static int go_file_counter = 0;

typedef struct {
    const char* src;
    size_t pos;
} GoJsonParser;

static void go_skip_ws(GoJsonParser* p) {
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos])) p->pos++;
}

static Value go_parse_val(GoJsonParser* p);

static Value go_parse_str(GoJsonParser* p) {
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

static Value go_parse_num(GoJsonParser* p) {
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

static Value go_parse_list(GoJsonParser* p) {
    p->pos++;
    Value res = val_list();
    ObjList* l = as_list(res);
    go_skip_ws(p);
    if (p->src[p->pos] == ']') { p->pos++; return res; }
    for (;;) {
        go_skip_ws(p);
        list_push(l, go_parse_val(p));
        go_skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == ']') { p->pos++; break; }
        break;
    }
    return res;
}

static Value go_parse_dict(GoJsonParser* p) {
    p->pos++;
    Value res = val_dict();
    ObjDict* d = as_dict(res);
    go_skip_ws(p);
    if (p->src[p->pos] == '}') { p->pos++; return res; }
    for (;;) {
        go_skip_ws(p);
        if (p->src[p->pos] != '"') break;
        Value k = go_parse_str(p);
        go_skip_ws(p);
        if (p->src[p->pos] == ':') p->pos++;
        go_skip_ws(p);
        Value v = go_parse_val(p);
        dict_set(d, k, v);
        go_skip_ws(p);
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        if (p->src[p->pos] == '}') { p->pos++; break; }
        break;
    }
    return res;
}

static Value go_parse_val(GoJsonParser* p) {
    go_skip_ws(p);
    char c = p->src[p->pos];
    if (!c) return val_nil();
    if (c == '"') return go_parse_str(p);
    if (c == '[') return go_parse_list(p);
    if (c == '{') return go_parse_dict(p);
    if (c == '-' || isdigit((unsigned char)c)) return go_parse_num(p);
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

static Value go_parse_output(const char* output) {
    if (!output || !*output) return val_nil();
    GoJsonParser p = { .src = output, .pos = 0 };
    return go_parse_val(&p);
}

static char* run_cmd_capture(const char* cmd, int* exit_code) {
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

static Value go_eval_internal(const char* expr) {
    int fid = ++go_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char go_file[2048];
    snprintf(go_file, sizeof(go_file), "%s/sky_go_eval_%d_%d.go", tmp_dir, (int)sky_getpid(), fid);

    FILE* f = fopen(go_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Go file");
        return val_nil();
    }

    fputs("package main\n\n", f);
    fputs("import (\n", f);
    fputs("    \"fmt\"\n", f);
    fputs("    \"math\"\n", f);
    fputs("    \"strings\"\n", f);
    fputs("    \"strconv\"\n", f);
    fputs("    \"time\"\n", f);
    fputs("    \"os\"\n", f);
    fputs("    \"sort\"\n", f);
    fputs("    \"encoding/json\"\n", f);
    fputs(")\n\n", f);
    fputs("var _ = fmt.Println\n", f);
    fputs("var _ = math.Pi\n", f);
    fputs("var _ = strings.ToUpper\n", f);
    fputs("var _ = strconv.Itoa\n", f);
    fputs("var _ = time.Now\n", f);
    fputs("var _ = os.Getenv\n", f);
    fputs("var _ = sort.Ints\n", f);
    fputs("var _ = json.Marshal\n\n", f);
    fputs("func main() {\n", f);
    fputs("    val := ", f);
    fputs(expr, f);
    fputs("\n", f);
    fputs("    data, err := json.Marshal(val)\n", f);
    fputs("    if err == nil && string(data) != \"\" && string(data) != \"null\" {\n", f);
    fputs("        fmt.Print(string(data))\n", f);
    fputs("    } else {\n", f);
    fputs("        fmt.Printf(\"%v\", val)\n", f);
    fputs("    }\n", f);
    fputs("}\n", f);
    fclose(f);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "go run \"%s\" 2>&1", go_file);
    int status = 0;
    char* out = run_cmd_capture(cmd, &status);
    sky_unlink(go_file);

    if (status != 0) {
        sky_runtime_error("GoError", "Go evaluation failed:\n%s", out);
        free(out);
        return val_nil();
    }

    Value result = go_parse_output(out);
    free(out);
    return result;
}

Value sky_go_exec(const char* code) {
    int fid = ++go_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char go_file[2048];
    snprintf(go_file, sizeof(go_file), "%s/sky_go_exec_%d_%d.go", tmp_dir, (int)sky_getpid(), fid);

    FILE* f = fopen(go_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Go file");
        return val_nil();
    }

    if (strstr(code, "package ") == NULL) {
        fputs("package main\n\n", f);
        fputs("import (\n", f);
        fputs("    \"fmt\"\n", f);
        fputs("    \"math\"\n", f);
        fputs("    \"strings\"\n", f);
        fputs("    \"time\"\n", f);
        fputs("    \"os\"\n", f);
        fputs("    \"strconv\"\n", f);
        fputs(")\n\n", f);
        fputs("var _ = fmt.Println\n", f);
        fputs("var _ = math.Pi\n", f);
        fputs("var _ = strings.ToUpper\n", f);
        fputs("var _ = time.Now\n", f);
        fputs("var _ = os.Getenv\n", f);
        fputs("var _ = strconv.Itoa\n\n", f);
        fputs("func main() {\n", f);
        fputs(code, f);
        fputs("\n}\n", f);
    } else {
        fputs(code, f);
    }
    fclose(f);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "go run \"%s\" 2>&1", go_file);
    int status = 0;
    char* out = run_cmd_capture(cmd, &status);
    sky_unlink(go_file);

    if (status != 0) {
        sky_runtime_error("GoError", "Go execution failed:\n%s", out);
        free(out);
        return val_nil();
    }

    Value res = val_string(out);
    free(out);
    return res;
}

Value sky_go_compile(const char* go_code) {
    int fid = ++go_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char go_file[2048];
    char so_file[2048];
    char h_file[2048];
    snprintf(go_file, sizeof(go_file), "%s/sky_go_%d_%d.go", tmp_dir, (int)sky_getpid(), fid);
    snprintf(so_file, sizeof(so_file), "%s/sky_go_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_SO_EXT);
    snprintf(h_file, sizeof(h_file), "%s/sky_go_%d_%d.h", tmp_dir, (int)sky_getpid(), fid);

    FILE* f = fopen(go_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary Go source file");
        return val_nil();
    }

    if (strstr(go_code, "package ") == NULL) {
        fputs("package main\n\nimport \"C\"\n\n", f);
        fputs(go_code, f);
        fputs("\nfunc main() {}\n", f);
    } else {
        fputs(go_code, f);
    }
    fclose(f);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "go build -buildmode=c-shared -o \"%s\" \"%s\" 2>&1", so_file, go_file);
    int status = 0;
    char* err = run_cmd_capture(cmd, &status);
    sky_unlink(go_file);
    sky_unlink(h_file);

    if (status != 0) {
        sky_unlink(so_file);
        sky_runtime_error("CompileError", "Go c-shared compilation failed:\n%s", err);
        free(err);
        return val_nil();
    }
    free(err);

    void* handle = sky_dlopen(so_file);
    if (!handle) {
        sky_runtime_error("OSError", "dlopen failed on compiled Go library: %s", sky_dlerror());
        return val_nil();
    }
    return val_foreign(FOREIGN_GO, "compiled_go", handle, NULL);
}

Value sky_go_load(const char* pkg_or_so) {
    if (strstr(pkg_or_so, ".so") != NULL || strstr(pkg_or_so, ".dylib") != NULL ||
        strstr(pkg_or_so, ".dll") != NULL || sky_access(pkg_or_so, F_OK) == 0) {
        void* handle = sky_dlopen(pkg_or_so);
        if (!handle) {
            sky_runtime_error("OSError", "Failed to open shared library '%s': %s", pkg_or_so, sky_dlerror());
            return val_nil();
        }
        return val_foreign(FOREIGN_GO, pkg_or_so, handle, NULL);
    }
    return val_foreign(FOREIGN_GO, pkg_or_so, strdup(pkg_or_so), NULL);
}

typedef double (*GoDoubleFn0)(void);
typedef double (*GoDoubleFn1)(double);
typedef double (*GoDoubleFn2)(double, double);
typedef double (*GoDoubleFn3)(double, double, double);
typedef double (*GoDoubleFn4)(double, double, double, double);

static double go_extract_double(Value v) {
    if (v.type == VAL_INT) return (double)v.as.i;
    if (v.type == VAL_DOUBLE) return v.as.d;
    return 0.0;
}

Value sky_go_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f) return val_nil();

    if (f->extra == NULL && f->handle && strstr(f->name, "compiled_go") != NULL) {
        void* sym = sky_dlsym(f->handle, name);
        if (!sym) {
            sky_runtime_error("AttributeError", "Symbol '%s' not found in Go shared library: %s", name, sky_dlerror());
            return val_nil();
        }
        switch (argc) {
            case 0: return val_double(((GoDoubleFn0)sym)());
            case 1: return val_double(((GoDoubleFn1)sym)(go_extract_double(argv[0])));
            case 2: return val_double(((GoDoubleFn2)sym)(go_extract_double(argv[0]), go_extract_double(argv[1])));
            case 3: return val_double(((GoDoubleFn3)sym)(go_extract_double(argv[0]), go_extract_double(argv[1]), go_extract_double(argv[2])));
            case 4: return val_double(((GoDoubleFn4)sym)(go_extract_double(argv[0]), go_extract_double(argv[1]), go_extract_double(argv[2]), go_extract_double(argv[3])));
            default:
                sky_runtime_error("TypeError", "Functions with > 4 arguments not supported via dynamic Go dispatcher");
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
            snprintf(arg_str, sizeof(arg_str), "nil");
        }
        if (i > 0) strncat(args_buf, ", ", sizeof(args_buf) - strlen(args_buf) - 1);
        strncat(args_buf, arg_str, sizeof(args_buf) - strlen(args_buf) - 1);
    }

    const char* pkg = f->name ? f->name : "math";
    snprintf(call_expr, sizeof(call_expr), "%s.%s(%s)", pkg, name, args_buf);
    return go_eval_internal(call_expr);
}

Value sky_go_get_prop(ObjForeign* f, const char* name) {
    if (!f) return val_nil();

    if (f->name && strcmp(f->name, "math") == 0) {
        if (strcmp(name, "Pi") == 0 || strcmp(name, "pi") == 0) return val_double(3.14159265358979323846);
        if (strcmp(name, "E") == 0 || strcmp(name, "e") == 0) return val_double(2.71828182845904523536);
        if (strcmp(name, "Phi") == 0) return val_double(1.61803398874989484820);
        if (strcmp(name, "Sqrt2") == 0) return val_double(1.41421356237309504880);
        if (strcmp(name, "Ln2") == 0) return val_double(0.693147180559945309417);
    }

    if (f->handle && strstr(f->name, "compiled_go") != NULL) {
        void* sym = sky_dlsym(f->handle, name);
        if (sym) return val_foreign(FOREIGN_GO, name, f->handle, sym);
    }

    char prop_expr[512];
    snprintf(prop_expr, sizeof(prop_expr), "%s.%s", f->name ? f->name : "", name);
    return go_eval_internal(prop_expr);
}

Value sky_go_set_prop(ObjForeign* f, const char* name, Value val) {
    (void)f; (void)name; (void)val;
    return val_nil();
}

static Value go_native_load(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    if (argc == 1) {
        if (argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
        return sky_go_load(((ObjString*)argv[0].as.obj)->chars);
    }
    Value list = val_list();
    ObjList* l = as_list(list);
    for (int i = 0; i < argc; ++i) {
        if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            list_push(l, sky_go_load(((ObjString*)argv[i].as.obj)->chars));
        }
    }
    return list;
}

static Value go_native_compile(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_go_compile(((ObjString*)argv[0].as.obj)->chars);
}

static Value go_native_exec(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_go_exec(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_go_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"),    val_function("load",    go_native_load,    -1));
    dict_set(d, val_string("compile"), val_function("compile", go_native_compile,  1));
    dict_set(d, val_string("exec"),    val_function("exec",    go_native_exec,     1));

    sky_mod_golang = mod;
    return mod;
}

