
#include "../include/sky_stdlib.h"
#include "../include/sky_platform.h"
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value sky_mod_math;
Value sky_mod_io;
Value sky_mod_fmt;
Value sky_mod_async;

static double to_num(Value v) {
    if (v.type == VAL_INT) return (double)v.as.i;
    if (v.type == VAL_DOUBLE) return v.as.d;
    return 0.0;
}

static const char* to_cstr(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_STRING) {
        return ((ObjString*)v.as.obj)->chars;
    }
    return "";
}

static Value math_sqrt(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(sqrt(to_num(argv[0])));
}
static Value math_sin(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(sin(to_num(argv[0])));
}
static Value math_cos(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(cos(to_num(argv[0])));
}
static Value math_tan(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(tan(to_num(argv[0])));
}
static Value math_asin(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(asin(to_num(argv[0])));
}
static Value math_acos(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(acos(to_num(argv[0])));
}
static Value math_atan(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(atan(to_num(argv[0])));
}
static Value math_atan2(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    return val_double(atan2(to_num(argv[0]), to_num(argv[1])));
}
static Value math_abs_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    if (argv[0].type == VAL_INT) {
        int64_t v = argv[0].as.i;
        return val_int(v < 0 ? -v : v);
    }
    return val_double(fabs(to_num(argv[0])));
}
static Value math_ceil_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(ceil(to_num(argv[0])));
}
static Value math_floor_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(floor(to_num(argv[0])));
}
static Value math_round_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(round(to_num(argv[0])));
}
static Value math_log_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(log(to_num(argv[0])));
}
static Value math_log10_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(log10(to_num(argv[0])));
}
static Value math_log2_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(log2(to_num(argv[0])));
}
static Value math_exp_fn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    return val_double(exp(to_num(argv[0])));
}
static Value math_pow_fn(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    return val_double(pow(to_num(argv[0]), to_num(argv[1])));
}
static Value math_min_fn(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    double a = to_num(argv[0]), b = to_num(argv[1]);
    if (argv[0].type == VAL_INT && argv[1].type == VAL_INT)
        return val_int(argv[0].as.i < argv[1].as.i ? argv[0].as.i : argv[1].as.i);
    return val_double(a < b ? a : b);
}
static Value math_max_fn(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    double a = to_num(argv[0]), b = to_num(argv[1]);
    if (argv[0].type == VAL_INT && argv[1].type == VAL_INT)
        return val_int(argv[0].as.i > argv[1].as.i ? argv[0].as.i : argv[1].as.i);
    return val_double(a > b ? a : b);
}
static Value math_hypot_fn(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    return val_double(hypot(to_num(argv[0]), to_num(argv[1])));
}
static Value math_clamp_fn(int argc, Value* argv) {
    if (argc < 3) return val_nil();
    double x = to_num(argv[0]), lo = to_num(argv[1]), hi = to_num(argv[2]);
    double r = x < lo ? lo : (x > hi ? hi : x);
    if (argv[0].type == VAL_INT && argv[1].type == VAL_INT && argv[2].type == VAL_INT)
        return val_int((int64_t)r);
    return val_double(r);
}

Value sky_stdlib_math_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("sqrt"),  val_function("sqrt",  math_sqrt,  1));
    dict_set(d, val_string("sin"),   val_function("sin",   math_sin,   1));
    dict_set(d, val_string("cos"),   val_function("cos",   math_cos,   1));
    dict_set(d, val_string("tan"),   val_function("tan",   math_tan,   1));
    dict_set(d, val_string("asin"),  val_function("asin",  math_asin,  1));
    dict_set(d, val_string("acos"),  val_function("acos",  math_acos,  1));
    dict_set(d, val_string("atan"),  val_function("atan",  math_atan,  1));
    dict_set(d, val_string("atan2"), val_function("atan2", math_atan2, 2));
    dict_set(d, val_string("abs"),   val_function("abs",   math_abs_fn,   1));
    dict_set(d, val_string("ceil"),  val_function("ceil",  math_ceil_fn,  1));
    dict_set(d, val_string("floor"), val_function("floor", math_floor_fn, 1));
    dict_set(d, val_string("round"), val_function("round", math_round_fn, 1));
    dict_set(d, val_string("log"),   val_function("log",   math_log_fn,   1));
    dict_set(d, val_string("log10"), val_function("log10", math_log10_fn, 1));
    dict_set(d, val_string("log2"),  val_function("log2",  math_log2_fn,  1));
    dict_set(d, val_string("exp"),   val_function("exp",   math_exp_fn,   1));
    dict_set(d, val_string("pow"),   val_function("pow",   math_pow_fn,   2));
    dict_set(d, val_string("min"),   val_function("min",   math_min_fn,   2));
    dict_set(d, val_string("max"),   val_function("max",   math_max_fn,   2));
    dict_set(d, val_string("hypot"), val_function("hypot", math_hypot_fn, 2));
    dict_set(d, val_string("clamp"), val_function("clamp", math_clamp_fn, 3));

    dict_set(d, val_string("pi"),  val_double(3.14159265358979323846));
    dict_set(d, val_string("e"),   val_double(2.71828182845904523536));
    dict_set(d, val_string("inf"), val_double(INFINITY));
    dict_set(d, val_string("nan"), val_double(NAN));
    return mod;
}

static Value io_readfile(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    const char* path = to_cstr(argv[0]);
    FILE* f = fopen(path, "rb");
    if (!f) {
        sky_runtime_error("FileNotFoundError", "[Errno 2] No such file or directory: '%s'", path);
        return val_nil();
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char* buf = (char*)GC_MALLOC(sz + 1);
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);
    return val_string(buf);
}

static Value io_writefile(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    const char* path = to_cstr(argv[0]);
    const char* data = to_cstr(argv[1]);
    FILE* f = fopen(path, "w");
    if (!f) {
        sky_runtime_error("IOError", "Cannot open '%s' for writing", path);
        return val_bool(false);
    }
    fputs(data, f);
    fclose(f);
    return val_bool(true);
}

static Value io_appendfile(int argc, Value* argv) {
    if (argc < 2) return val_nil();
    const char* path = to_cstr(argv[0]);
    const char* data = to_cstr(argv[1]);
    FILE* f = fopen(path, "a");
    if (!f) {
        sky_runtime_error("IOError", "Cannot open '%s' for appending", path);
        return val_bool(false);
    }
    fputs(data, f);
    fclose(f);
    return val_bool(true);
}

static Value io_readlines(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    const char* path = to_cstr(argv[0]);
    FILE* f = fopen(path, "r");
    if (!f) {
        sky_runtime_error("FileNotFoundError", "[Errno 2] No such file or directory: '%s'", path);
        return val_list();
    }
    Value result = val_list();
    ObjList* list = as_list(result);
    char line[4096];
    while (fgets(line, sizeof(line), f)) {

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        list_push(list, val_string(line));
    }
    fclose(f);
    return result;
}

static Value io_input(int argc, Value* argv) {
    if (argc >= 1) {
        const char* prompt = to_cstr(argv[0]);
        printf("%s", prompt);
        fflush(stdout);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return val_nil();
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return val_string(buf);
}

static Value io_exists(int argc, Value* argv) {
    if (argc < 1) return val_bool(false);
    return val_bool(sky_access(to_cstr(argv[0]), F_OK) == 0);
}

static Value io_remove_fn(int argc, Value* argv) {
    if (argc < 1) return val_bool(false);
    return val_bool(remove(to_cstr(argv[0])) == 0);
}

Value sky_stdlib_io_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("readfile"),   val_function("readfile",   io_readfile,   1));
    dict_set(d, val_string("writefile"),  val_function("writefile",  io_writefile,  2));
    dict_set(d, val_string("appendfile"), val_function("appendfile", io_appendfile, 2));
    dict_set(d, val_string("readlines"),  val_function("readlines",  io_readlines,  1));
    dict_set(d, val_string("input"),      val_function("input",      io_input,      1));
    dict_set(d, val_string("exists"),     val_function("exists",     io_exists,     1));
    dict_set(d, val_string("remove"),     val_function("remove",     io_remove_fn,  1));
    return mod;
}

static Value fmt_format(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    const char* tmpl = to_cstr(argv[0]);
    size_t cap = 256, len = 0;
    char* out = (char*)GC_MALLOC(cap);
    int arg_idx = 1;
    for (const char* p = tmpl; *p; ++p) {
        if (*p == '{' && *(p + 1) == '}') {

            char* s = (arg_idx < argc) ? val_to_string(argv[arg_idx++]) : strdup("");
            size_t sl = strlen(s);
            while (len + sl + 1 >= cap) { cap *= 2; out = (char*)GC_REALLOC(out, cap); }
            memcpy(out + len, s, sl);
            len += sl;
            free(s);
            p++;
        } else {
            if (len + 2 >= cap) { cap *= 2; out = (char*)GC_REALLOC(out, cap); }
            out[len++] = *p;
        }
    }
    out[len] = '\0';
    return val_string(out);
}

static Value fmt_hex(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_INT) return val_string("0");
    char buf[32];
    snprintf(buf, sizeof(buf), "%llx", (long long)argv[0].as.i);
    return val_string(buf);
}

static Value fmt_bin(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_INT) return val_string("0");
    int64_t n = argv[0].as.i;
    if (n == 0) return val_string("0");
    char buf[66];
    int pos = 65;
    buf[pos--] = '\0';
    uint64_t u = (uint64_t)(n < 0 ? -n : n);
    while (u > 0) { buf[pos--] = '0' + (u & 1); u >>= 1; }
    if (n < 0) buf[pos--] = '-';
    return val_string(buf + pos + 1);
}

static Value fmt_oct(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_INT) return val_string("0");
    char buf[32];
    snprintf(buf, sizeof(buf), "%llo", (long long)argv[0].as.i);
    return val_string(buf);
}

static Value fmt_pad(int argc, Value* argv) {
    if (argc < 2) return (argc >= 1) ? argv[0] : val_string("");
    const char* s = to_cstr(argv[0]);
    int width = (argv[1].type == VAL_INT) ? (int)argv[1].as.i : 0;
    int slen = (int)strlen(s);
    if (slen >= width) return val_string(s);
    char* buf = (char*)GC_MALLOC(width + 1);
    memcpy(buf, s, slen);
    memset(buf + slen, ' ', width - slen);
    buf[width] = '\0';
    return val_string(buf);
}

static Value fmt_padleft(int argc, Value* argv) {
    if (argc < 2) return (argc >= 1) ? argv[0] : val_string("");
    const char* s = to_cstr(argv[0]);
    int width = (argv[1].type == VAL_INT) ? (int)argv[1].as.i : 0;
    int slen = (int)strlen(s);
    if (slen >= width) return val_string(s);
    char* buf = (char*)GC_MALLOC(width + 1);
    int pad = width - slen;
    memset(buf, ' ', pad);
    memcpy(buf + pad, s, slen);
    buf[width] = '\0';
    return val_string(buf);
}

static Value fmt_repeat(int argc, Value* argv) {
    if (argc < 2) return (argc >= 1) ? argv[0] : val_string("");
    const char* s = to_cstr(argv[0]);
    int n = (argv[1].type == VAL_INT) ? (int)argv[1].as.i : 0;
    if (n <= 0) return val_string("");
    size_t slen = strlen(s);
    char* buf = (char*)GC_MALLOC(slen * n + 1);
    for (int i = 0; i < n; ++i) memcpy(buf + i * slen, s, slen);
    buf[slen * n] = '\0';
    return val_string(buf);
}

Value sky_stdlib_fmt_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("format"),  val_function("format",  fmt_format,  -1));
    dict_set(d, val_string("hex"),     val_function("hex",     fmt_hex,     1));
    dict_set(d, val_string("bin"),     val_function("bin",     fmt_bin,     1));
    dict_set(d, val_string("oct"),     val_function("oct",     fmt_oct,     1));
    dict_set(d, val_string("pad"),     val_function("pad",     fmt_pad,     2));
    dict_set(d, val_string("padleft"), val_function("padleft", fmt_padleft, 2));
    dict_set(d, val_string("repeat"),  val_function("repeat",  fmt_repeat,  2));
    return mod;
}

Value sky_stdlib_async_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("sleep"), val_function("sleep", sky_async_sleep, 1));
    dict_set(d, val_string("all"),   val_function("all",   sky_async_all,   1));
    dict_set(d, val_string("race"),  val_function("race",  sky_async_race,  1));
    dict_set(d, val_string("spawn"), val_function("spawn", sky_async_spawn, -1));
    return mod;
}

void sky_stdlib_init_all(void) {
    sky_mod_math  = sky_stdlib_math_init();
    sky_mod_io    = sky_stdlib_io_init();
    sky_mod_fmt   = sky_stdlib_fmt_init();
    sky_mod_async = sky_stdlib_async_init();
}

