#include "../include/skylang.h"
#include "../include/sky_cpp.h"
#include "../include/sky_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Value sky_mod_cpp;
static int cpp_file_counter = 0;

Value sky_cpp_load(const char* so_path) {
    void* handle = sky_dlopen(so_path);
    if (!handle) {
        sky_runtime_error("OSError", "Failed to open shared library '%s': %s", so_path, sky_dlerror());
        return val_nil();
    }
    return val_foreign(FOREIGN_CPP, so_path, handle, NULL);
}

void sky_extract_cpp_declarations(const char* code, const char* file, int base_line, ForeignSymbolTable* table) {
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
        if (*p == '"' || *p == '\'') {
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

        if (brace_depth == 0) {
            if (strncmp(p, "extern", 6) == 0 && isspace((unsigned char)p[6])) {
                p += 6;
                while (*p && isspace((unsigned char)*p)) p++;
                if (strncmp(p, "\"C\"", 3) == 0) {
                    p += 3;
                    while (*p && isspace((unsigned char)*p)) p++;
                    if (*p == '{') {
                        p++;
                        continue;
                    }
                }
            }

            if (isalpha((unsigned char)*p) || *p == '_') {
                while (isalnum((unsigned char)*p) || *p == '_' || *p == ':' || *p == '<' || *p == '>' || *p == '*' || *p == '&') {
                    if (*p == '<') {
                        int tdepth = 1;
                        p++;
                        while (*p && tdepth > 0) {
                            if (*p == '<') tdepth++;
                            else if (*p == '>') tdepth--;
                            p++;
                        }
                    } else {
                        p++;
                    }
                }
                while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }

                while (isalpha((unsigned char)*p) || *p == '_') {
                    const char* id_start = p;
                    while (isalnum((unsigned char)*p) || *p == '_') p++;
                    size_t id_len = (size_t)(p - id_start);
                    char name[128];
                    if (id_len >= sizeof(name)) id_len = sizeof(name) - 1;
                    memcpy(name, id_start, id_len);
                    name[id_len] = '\0';

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }

                    if (*p == '[') {
                        while (*p && *p != ']') p++;
                        if (*p == ']') p++;
                        foreign_symtable_add(table, name, "cpp", FOREIGN_DECL_VAR, file, cur_line);
                    } else if (*p == '(') {
                        foreign_symtable_add(table, name, "cpp", FOREIGN_DECL_FUNCTION, file, cur_line);
                        int pdepth = 1;
                        p++;
                        while (*p && pdepth > 0) {
                            if (*p == '(') pdepth++;
                            else if (*p == ')') pdepth--;
                            p++;
                        }
                        while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                        if (*p == '{') {
                            int bdepth = 1;
                            p++;
                            while (*p && bdepth > 0) {
                                if (*p == '{') bdepth++;
                                else if (*p == '}') bdepth--;
                                p++;
                            }
                        }
                        break;
                    } else {
                        foreign_symtable_add(table, name, "cpp", FOREIGN_DECL_VAR, file, cur_line);
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
                            else if (*p == '"' || *p == '\'') {
                                char q = *p++;
                                while (*p && *p != q) {
                                    if (*p == '\\' && *(p + 1)) p += 2;
                                    else p++;
                                }
                                if (*p == q) p++;
                                continue;
                            } else if (paren == 0 && bracket == 0 && brace == 0) {
                                if (*p == ',' || *p == ';' || *p == '\n') break;
                            }
                            p++;
                        }
                    }

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (*p == ',') {
                        p++;
                        while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                        continue;
                    }
                    if (*p == ';') { p++; break; }
                    break;
                }
                continue;
            }
        }

        p++;
    }
}

static Value parse_bracket_or_brace_list(const char** pp) {
    const char* p = *pp;
    if (*p != '{' && *p != '[') return val_nil();
    char open_ch = *p;
    char close_ch = (open_ch == '{') ? '}' : ']';
    p++;

    Value list_val = val_list();
    ObjList* l = as_list(list_val);

    while (*p && *p != close_ch) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == close_ch || !*p) break;

        if (*p == '{' || *p == '[') {
            list_push(l, parse_bracket_or_brace_list(&p));
        } else if (*p == '"' || *p == '\'') {
            char q = *p++;
            const char* str_start = p;
            while (*p && *p != q) {
                if (*p == '\\' && *(p + 1)) p += 2;
                else p++;
            }
            size_t slen = (size_t)(p - str_start);
            char* sval = (char*)malloc(slen + 1);
            memcpy(sval, str_start, slen);
            sval[slen] = '\0';
            list_push(l, val_string(sval));
            free(sval);
            if (*p == q) p++;
        } else {
            const char* val_start = p;
            while (*p && *p != ',' && *p != close_ch && !isspace((unsigned char)*p)) p++;
            size_t vlen = (size_t)(p - val_start);
            char vstr[64];
            if (vlen >= sizeof(vstr)) vlen = sizeof(vstr) - 1;
            memcpy(vstr, val_start, vlen);
            vstr[vlen] = '\0';
            if (strcmp(vstr, "true") == 0) {
                list_push(l, val_bool(true));
            } else if (strcmp(vstr, "false") == 0) {
                list_push(l, val_bool(false));
            } else if (strchr(vstr, '.') || strchr(vstr, 'e') || strchr(vstr, 'E') ||
                       strchr(vstr, 'f') || strchr(vstr, 'F') || strchr(vstr, 'd') || strchr(vstr, 'D')) {
                list_push(l, val_double(atof(vstr)));
            } else {
                list_push(l, val_int(atoll(vstr)));
            }
        }
    }
    if (*p == close_ch) p++;
    *pp = p;
    return list_val;
}

Value sky_cpp_compile(const char* cpp_code) {
    int fid = ++cpp_file_counter;
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));

    char cpp_file[2048];
    char so_file[2048];
    snprintf(cpp_file, sizeof(cpp_file), "%s/sky_cpp_%d_%d.cpp", tmp_dir, (int)sky_getpid(), fid);
    snprintf(so_file, sizeof(so_file), "%s/sky_cpp_%d_%d%s", tmp_dir, (int)sky_getpid(), fid, SKY_SO_EXT);

    FILE* f = fopen(cpp_file, "w");
    if (!f) {
        sky_runtime_error("IOError", "Failed to create temporary C++ source file");
        return val_nil();
    }

    fputs("#include <iostream>\n", f);
    fputs("#include <vector>\n", f);
    fputs("#include <string>\n", f);
    fputs("#include <cmath>\n", f);
    fputs("#include <algorithm>\n", f);
    fputs("#include <map>\n", f);
    fputs("#include <memory>\n\n", f);
    fputs(cpp_code, f);
    fclose(f);

    char cmd[8192];
#if defined(SKY_OS_WINDOWS)
    snprintf(cmd, sizeof(cmd), "g++ -O2 -shared -std=c++17 \"%s\" -o \"%s\" 2>&1", cpp_file, so_file);
#elif defined(SKY_OS_MACOS)
    snprintf(cmd, sizeof(cmd), "g++ -O2 -dynamiclib -std=c++17 \"%s\" -o \"%s\" 2>&1", cpp_file, so_file);
#else
    snprintf(cmd, sizeof(cmd), "g++ -O2 -shared -fPIC -std=c++17 \"%s\" -o \"%s\" 2>&1", cpp_file, so_file);
#endif
    FILE* pipe = popen(cmd, "r");
    if (pipe) {
        char err_buf[512];
        char full_err[4096] = "";
        while (fgets(err_buf, sizeof(err_buf), pipe)) {
            strncat(full_err, err_buf, sizeof(full_err) - strlen(full_err) - 1);
        }
        int status = pclose(pipe);
        if (status != 0) {
            sky_unlink(cpp_file);
            sky_runtime_error("CompileError", "C++ inline compilation failed:\n%s", full_err);
            return val_nil();
        }
    }
    sky_unlink(cpp_file);

    void* handle = sky_dlopen(so_file);
    if (!handle) {
        sky_runtime_error("OSError", "dlopen failed on compiled C++ library: %s", sky_dlerror());
        return val_nil();
    }

    ForeignSymbolTable syms;
    foreign_symtable_init(&syms);
    sky_extract_cpp_declarations(cpp_code, "<cpp>", 1, &syms);

    Value result_dict = val_dict();
    ObjDict* d = as_dict(result_dict);

    for (size_t i = 0; i < syms.count; ++i) {
        const char* sname = syms.items[i].name;
        void* sym = sky_dlsym(handle, sname);
        if (sym && syms.items[i].kind == FOREIGN_DECL_FUNCTION) {
            dict_set(d, val_string(sname), val_foreign(FOREIGN_CPP, sname, handle, sym));
            continue;
        }

        const char* p = strstr(cpp_code, sname);
        if (p) {
            p += strlen(sname);
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '[') {
                while (*p && *p != ']') p++;
                if (*p == ']') p++;
                while (*p && isspace((unsigned char)*p)) p++;
            }
            if (*p == '=') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == '{' || *p == '[') {
                    Value list_val = parse_bracket_or_brace_list(&p);
                    dict_set(d, val_string(sname), list_val);
                } else if (*p == '"') {
                    p++;
                    const char* str_start = p;
                    while (*p && *p != '"') {
                        if (*p == '\\' && *(p+1)) p += 2;
                        else p++;
                    }
                    size_t slen = (size_t)(p - str_start);
                    char* sval = (char*)malloc(slen + 1);
                    memcpy(sval, str_start, slen);
                    sval[slen] = '\0';
                    dict_set(d, val_string(sname), val_string(sval));
                    free(sval);
                } else {
                    const char* val_start = p;
                    while (*p && *p != ';' && *p != ',' && !isspace((unsigned char)*p)) p++;
                    size_t vlen = (size_t)(p - val_start);
                    char vstr[64];
                    if (vlen >= sizeof(vstr)) vlen = sizeof(vstr) - 1;
                    memcpy(vstr, val_start, vlen);
                    vstr[vlen] = '\0';
                    if (strcmp(vstr, "true") == 0) {
                        dict_set(d, val_string(sname), val_bool(true));
                    } else if (strcmp(vstr, "false") == 0) {
                        dict_set(d, val_string(sname), val_bool(false));
                    } else if (strchr(vstr, '.') || strchr(vstr, 'e') || strchr(vstr, 'E') ||
                               strchr(vstr, 'f') || strchr(vstr, 'F') || strchr(vstr, 'd') || strchr(vstr, 'D')) {
                        dict_set(d, val_string(sname), val_double(atof(vstr)));
                    } else {
                        dict_set(d, val_string(sname), val_int(atoll(vstr)));
                    }
                }
            }
        }
    }

    dict_set(d, val_string("__foreign_handle__"), val_foreign(FOREIGN_CPP, "compiled_cpp", handle, NULL));
    foreign_symtable_free(&syms);
    return result_dict;
}

Value sky_cpp_exec(const char* cpp_code) {
    return sky_cpp_compile(cpp_code);
}

typedef double (*DoubleFn0)(void);
typedef double (*DoubleFn1)(double);
typedef double (*DoubleFn2)(double, double);
typedef double (*DoubleFn3)(double, double, double);
typedef double (*DoubleFn4)(double, double, double, double);

typedef int64_t (*IntFn0)(void);
typedef int64_t (*IntFn1)(int64_t);
typedef int64_t (*IntFn2)(int64_t, int64_t);

static double extract_double(Value v) {
    if (v.type == VAL_INT) return (double)v.as.i;
    if (v.type == VAL_DOUBLE) return v.as.d;
    return 0.0;
}

Value sky_cpp_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f || !f->handle) return val_nil();
    void* sym = sky_dlsym(f->handle, name);
    if (!sym) {
        sky_runtime_error("AttributeError", "Symbol '%s' not found in %s: %s",
                          name, f->name ? f->name : "library", sky_dlerror());
        return val_nil();
    }

    switch (argc) {
        case 0: {
            DoubleFn0 fn = (DoubleFn0)sym;
            return val_double(fn());
        }
        case 1: {
            DoubleFn1 fn = (DoubleFn1)sym;
            return val_double(fn(extract_double(argv[0])));
        }
        case 2: {
            DoubleFn2 fn = (DoubleFn2)sym;
            return val_double(fn(extract_double(argv[0]), extract_double(argv[1])));
        }
        case 3: {
            DoubleFn3 fn = (DoubleFn3)sym;
            return val_double(fn(extract_double(argv[0]), extract_double(argv[1]), extract_double(argv[2])));
        }
        case 4: {
            DoubleFn4 fn = (DoubleFn4)sym;
            return val_double(fn(extract_double(argv[0]), extract_double(argv[1]), extract_double(argv[2]), extract_double(argv[3])));
        }
        default:
            sky_runtime_error("TypeError", "Functions with > 4 arguments not supported via dynamic C++ dispatcher");
            return val_nil();
    }
}

Value sky_cpp_get_prop(ObjForeign* f, const char* name) {
    if (!f || !f->handle) return val_nil();
    void* sym = sky_dlsym(f->handle, name);
    if (!sym) return val_nil();
    return val_foreign(FOREIGN_CPP, name, f->handle, sym);
}

Value sky_cpp_set_prop(ObjForeign* f, const char* name, Value val) {
    (void)f; (void)name; (void)val;
    return val_nil();
}

static Value cpp_native_load(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_cpp_load(((ObjString*)argv[0].as.obj)->chars);
}

static Value cpp_native_compile(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_cpp_compile(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_cpp_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"),    val_function("load",    cpp_native_load,    1));
    dict_set(d, val_string("compile"), val_function("compile", cpp_native_compile, 1));
    dict_set(d, val_string("exec"),    val_function("exec",    cpp_native_compile, 1));

    sky_mod_cpp = mod;
    return mod;
}

