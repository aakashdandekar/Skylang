
#include "../include/sky_cpp.h"
#include "../include/sky_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

Value sky_mod_cpp;

Value sky_cpp_load(const char* so_path) {
    void* handle = sky_dlopen(so_path);
    if (!handle) {
        sky_runtime_error("OSError", "Failed to open shared library '%s': %s", so_path, sky_dlerror());
        return val_nil();
    }
    return val_foreign(FOREIGN_CPP, so_path, handle, NULL);
}

static int cpp_file_counter = 0;

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
    return val_foreign(FOREIGN_CPP, "compiled_cpp", handle, NULL);
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

    sky_mod_cpp = mod;
    return mod;
}

