#define _GNU_SOURCE
#include "../include/skylang_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include "../include/sky_python.h"
#include "../include/sky_js.h"
#include "../include/sky_cpp.h"
#include "../include/sky_java.h"
#include "../include/sky_go.h"
#include "../include/sky_rust.h"

__thread const char* sky_current_file = "<main>";
__thread int sky_current_line = 1;
__thread const char* sky_current_fn = "<main>";

#define SKY_MAX_CALL_STACK 1024

typedef struct {
    const char* file;
    int line;
    const char* fn_name;
} SkyCallFrame;

static __thread SkyCallFrame sky_call_stack[SKY_MAX_CALL_STACK];
static __thread int sky_call_depth = 0;

void sky_set_loc(const char* file, int line, const char* fn) {
    if (file && file[0] != '\0') sky_current_file = file;
    if (line > 0) sky_current_line = line;
    if (fn && fn[0] != '\0') sky_current_fn = fn;
}

void sky_push_frame(const char* file, int line, const char* fn) {
    if (sky_call_depth < SKY_MAX_CALL_STACK) {
        sky_call_stack[sky_call_depth].file = (file && file[0] != '\0') ? file : sky_current_file;
        sky_call_stack[sky_call_depth].line = line > 0 ? line : sky_current_line;
        sky_call_stack[sky_call_depth].fn_name = (fn && fn[0] != '\0') ? fn : sky_current_fn;
        sky_call_depth++;
    }
}

void sky_pop_frame(void) {
    if (sky_call_depth > 0) {
        sky_call_depth--;
    }
}

static void print_source_line(const char* file, int line) {
    if (!file || line <= 0) return;
    FILE* f = fopen(file, "r");
    if (!f) return;

    char buf[1024];
    int cur = 1;
    while (fgets(buf, sizeof(buf), f)) {
        if (cur == line) {
            size_t len = strlen(buf);
            while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
                buf[--len] = '\0';
            }
            char* trimmed = buf;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
            fprintf(stderr, "    %s\n", trimmed);
            break;
        }
        cur++;
    }
    fclose(f);
}

void sky_runtime_error(const char* exc_name, const char* fmt, ...) {
    char msg[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    fprintf(stderr, "Traceback (most recent call last):\n");
    if (sky_call_depth > 0) {
        for (int i = 0; i < sky_call_depth; i++) {
            fprintf(stderr, "  File \"%s\", line %d, in %s\n",
                    sky_call_stack[i].file ? sky_call_stack[i].file : "<main>",
                    sky_call_stack[i].line,
                    sky_call_stack[i].fn_name ? sky_call_stack[i].fn_name : "<main>");
            print_source_line(sky_call_stack[i].file, sky_call_stack[i].line);
        }
    }
    fprintf(stderr, "  File \"%s\", line %d, in %s\n",
            sky_current_file, sky_current_line, sky_current_fn ? sky_current_fn : "<main>");
    print_source_line(sky_current_file, sky_current_line);

    fprintf(stderr, "%s: %s\n", exc_name ? exc_name : "RuntimeError", msg);
    exit(1);
}

void sky_init_runtime(void) {
    static bool initialized = false;
    if (!initialized) {
        GC_INIT();
        initialized = true;
    }
}

static void* sky_alloc(size_t bytes) {
    void* ptr = GC_MALLOC(bytes);
    if (!ptr) {
        sky_runtime_error("MemoryError", "out of memory allocating %zu bytes", bytes);
        exit(1);
    }
    return ptr;
}

Value val_nil(void) {
    Value v;
    v.type = VAL_NIL;
    v.as.i = 0;
    return v;
}

Value val_int(int64_t v) {
    Value val;
    val.type = VAL_INT;
    val.as.i = v;
    return val;
}

Value val_double(double v) {
    Value val;
    val.type = VAL_DOUBLE;
    val.as.d = v;
    return val;
}

Value val_bool(bool v) {
    Value val;
    val.type = VAL_BOOL;
    val.as.b = v;
    return val;
}

Value val_char(char v) {
    Value val;
    val.type = VAL_CHAR;
    val.as.c = v;
    return val;
}

Value val_type(SkyTypeId t) {
    Value val;
    val.type = VAL_TYPE;
    val.as.t = t;
    return val;
}

Value val_string(const char* str) {
    if (!str) str = "";
    return val_string_len(str, strlen(str));
}

Value val_string_len(const char* str, size_t len) {
    ObjString* os = (ObjString*)sky_alloc(sizeof(ObjString));
    os->header.type = OBJ_STRING;
    os->length = len;
    os->chars = (char*)sky_alloc(len + 1);
    if (str) {
        memcpy(os->chars, str, len);
    }
    os->chars[len] = '\0';

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)os;
    return v;
}

Value val_error(const char* message) {
    ObjError* e = (ObjError*)sky_alloc(sizeof(ObjError));
    e->header.type = OBJ_ERROR;
    size_t len = message ? strlen(message) : 0;
    e->message = (char*)sky_alloc(len + 1);
    if (message) memcpy(e->message, message, len);
    e->message[len] = '\0';

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)e;
    return v;
}

Value val_array(size_t capacity, SkyTypeId elem_type) {
    ObjArray* arr = (ObjArray*)sky_alloc(sizeof(ObjArray));
    arr->header.type = OBJ_ARRAY;
    arr->capacity = capacity;
    arr->elem_type = elem_type;
    arr->items = (Value*)sky_alloc(sizeof(Value) * (capacity > 0 ? capacity : 1));

    Value def_val;
    switch (elem_type) {
        case TYPE_INT: def_val = val_int(0); break;
        case TYPE_DOUBLE: def_val = val_double(0.0); break;
        case TYPE_BOOL: def_val = val_bool(false); break;
        case TYPE_CHAR: def_val = val_char('a'); break;
        case TYPE_STRING: def_val = val_string(""); break;
        default: def_val = val_nil(); break;
    }
    for (size_t i = 0; i < capacity; ++i) {
        arr->items[i] = def_val;
    }

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)arr;
    return v;
}

Value val_list(void) {
    ObjList* list = (ObjList*)sky_alloc(sizeof(ObjList));
    list->header.type = OBJ_LIST;
    list->capacity = 8;
    list->count = 0;
    list->items = (Value*)sky_alloc(sizeof(Value) * list->capacity);

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)list;
    return v;
}

Value val_tuple(size_t count, Value* items) {
    ObjTuple* tuple = (ObjTuple*)sky_alloc(sizeof(ObjTuple));
    tuple->header.type = OBJ_TUPLE;
    tuple->count = count;
    tuple->items = (Value*)sky_alloc(sizeof(Value) * (count > 0 ? count : 1));
    if (items && count > 0) {
        memcpy(tuple->items, items, sizeof(Value) * count);
    }

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)tuple;
    return v;
}

Value val_dict(void) {
    ObjDict* d = (ObjDict*)sky_alloc(sizeof(ObjDict));
    d->header.type = OBJ_DICT;
    d->capacity = 16;
    d->count = 0;
    d->entries = (DictEntry*)sky_alloc(sizeof(DictEntry) * d->capacity);
    for (size_t i = 0; i < d->capacity; ++i) {
        d->entries[i].occupied = false;
        d->entries[i].key = val_nil();
        d->entries[i].value = val_nil();
    }

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)d;
    return v;
}

Value val_set(void) {
    ObjSet* s = (ObjSet*)sky_alloc(sizeof(ObjSet));
    s->header.type = OBJ_SET;
    s->capacity = 16;
    s->count = 0;
    s->entries = (SetEntry*)sky_alloc(sizeof(SetEntry) * s->capacity);
    for (size_t i = 0; i < s->capacity; ++i) {
        s->entries[i].occupied = false;
        s->entries[i].item = val_nil();
    }

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)s;
    return v;
}

Value val_sorted_list(void) {
    ObjSortedList* sl = (ObjSortedList*)sky_alloc(sizeof(ObjSortedList));
    sl->header.type = OBJ_SORTED_LIST;
    sl->capacity = 8;
    sl->count = 0;
    sl->items = (Value*)sky_alloc(sizeof(Value) * sl->capacity);

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)sl;
    return v;
}

Value val_class(const char* name) {
    ObjClass* k = (ObjClass*)sky_alloc(sizeof(ObjClass));
    k->header.type = OBJ_CLASS;
    k->name = name;
    k->methods = NULL;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)k;
    return v;
}

Value val_instance(ObjClass* klass) {
    ObjInstance* inst = (ObjInstance*)sky_alloc(sizeof(ObjInstance));
    inst->header.type = OBJ_INSTANCE;
    inst->klass = klass;
    inst->fields = NULL;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)inst;
    return v;
}

Value val_foreign(ForeignLang lang, const char* name, void* handle, void* extra) {
    ObjForeign* f = (ObjForeign*)sky_alloc(sizeof(ObjForeign));
    f->header.type = OBJ_FOREIGN;
    f->lang = lang;
    f->name = name ? strdup(name) : NULL;
    f->handle = handle;
    f->extra = extra;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)f;
    return v;
}

Value val_function(const char* name, SkyNativeFn fn, int arity) {
    ObjFunction* f = (ObjFunction*)sky_alloc(sizeof(ObjFunction));
    f->header.type = OBJ_FUNCTION;
    f->name = name;
    f->fn = fn;
    f->arity = arity;
    f->param_count = 0;
    f->param_names = NULL;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)f;
    return v;
}

Value val_function_with_params(const char* name, SkyNativeFn fn, int arity, int param_count, const char** param_names) {
    ObjFunction* f = (ObjFunction*)sky_alloc(sizeof(ObjFunction));
    f->header.type = OBJ_FUNCTION;
    f->name = name;
    f->fn = fn;
    f->arity = arity;
    f->param_count = param_count;
    f->param_names = param_names;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)f;
    return v;
}

ObjString* as_string(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_STRING) return (ObjString*)v.as.obj;
    return NULL;
}
ObjArray* as_array(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_ARRAY) return (ObjArray*)v.as.obj;
    return NULL;
}
ObjList* as_list(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_LIST) return (ObjList*)v.as.obj;
    return NULL;
}
ObjTuple* as_tuple(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_TUPLE) return (ObjTuple*)v.as.obj;
    return NULL;
}
ObjDict* as_dict(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_DICT) return (ObjDict*)v.as.obj;
    return NULL;
}
ObjSet* as_set(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_SET) return (ObjSet*)v.as.obj;
    return NULL;
}
ObjSortedList* as_sorted_list(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_SORTED_LIST) return (ObjSortedList*)v.as.obj;
    return NULL;
}
ObjClass* as_class(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_CLASS) return (ObjClass*)v.as.obj;
    return NULL;
}
ObjInstance* as_instance(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_INSTANCE) return (ObjInstance*)v.as.obj;
    return NULL;
}
ObjForeign* as_foreign(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_FOREIGN) return (ObjForeign*)v.as.obj;
    return NULL;
}

ObjFuture* as_future(Value v) {
    if (v.type == VAL_OBJ && v.as.obj->type == OBJ_FUTURE) return (ObjFuture*)v.as.obj;
    return NULL;
}

Value val_future(SkyFuture* fut) {
    ObjFuture* f = (ObjFuture*)sky_alloc(sizeof(ObjFuture));
    f->header.type = OBJ_FUTURE;
    f->future = fut;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)f;
    return v;
}

SkyFuture* sky_future_create(void) {
    SkyFuture* fut = (SkyFuture*)sky_alloc(sizeof(SkyFuture));
    pthread_mutex_init(&fut->mutex, NULL);
    pthread_cond_init(&fut->cond, NULL);
    fut->state = FUTURE_PENDING;
    fut->result = val_nil();
    fut->error_msg = NULL;
    fut->has_thread = false;
    fut->is_joined = false;
    return fut;
}

void sky_future_resolve(SkyFuture* fut, Value res) {
    if (!fut) return;
    pthread_mutex_lock(&fut->mutex);
    fut->state = FUTURE_RESOLVED;
    fut->result = res;
    pthread_cond_broadcast(&fut->cond);
    pthread_mutex_unlock(&fut->mutex);
}

void sky_future_reject(SkyFuture* fut, const char* err) {
    if (!fut) return;
    pthread_mutex_lock(&fut->mutex);
    fut->state = FUTURE_REJECTED;
    fut->error_msg = err ? strdup(err) : strdup("Unknown async error");
    pthread_cond_broadcast(&fut->cond);
    pthread_mutex_unlock(&fut->mutex);
}

Value sky_future_await(SkyFuture* fut) {
    if (!fut) return val_nil();
    if (fut->has_thread && !fut->is_joined) {
        pthread_join(fut->thread, NULL);
        fut->is_joined = true;
    }
    pthread_mutex_lock(&fut->mutex);
    while (fut->state == FUTURE_PENDING || fut->state == FUTURE_RUNNING) {
        pthread_cond_wait(&fut->cond, &fut->mutex);
    }
    FutureState st = fut->state;
    char* err_msg = fut->error_msg ? strdup(fut->error_msg) : NULL;
    Value res = fut->result;
    pthread_mutex_unlock(&fut->mutex);

    if (st == FUTURE_REJECTED) {
        sky_runtime_error("AsyncError", "%s", err_msg ? err_msg : "Async operation failed");
        return val_nil();
    }
    return res;
}

Value val_future_await(Value v) {
    ObjFuture* of = as_future(v);
    if (!of || !of->future) {
        return v;
    }
    return sky_future_await(of->future);
}

typedef struct {
    SkyFuture* future;
    double seconds;
} AsyncSleepArgs;

static void* __async_sleep_worker(void* raw_args) {
    AsyncSleepArgs* args = (AsyncSleepArgs*)raw_args;
    double sec = args->seconds;
    if (sec > 0.0) {
#if defined(SKY_OS_WINDOWS)
        Sleep((DWORD)(sec * 1000.0));
#else
        usleep((useconds_t)(sec * 1000000.0));
#endif
    }
    sky_future_resolve(args->future, val_double(sec));
    return NULL;
}

Value sky_async_sleep(int argc, Value* argv) {
    double sec = 0.0;
    if (argc >= 1) {
        if (argv[0].type == VAL_INT) sec = (double)argv[0].as.i;
        else if (argv[0].type == VAL_DOUBLE) sec = argv[0].as.d;
    }
    SkyFuture* fut = sky_future_create();
    fut->has_thread = true;
    AsyncSleepArgs* args = (AsyncSleepArgs*)sky_alloc(sizeof(AsyncSleepArgs));
    args->future = fut;
    args->seconds = sec;
    pthread_create(&fut->thread, NULL, __async_sleep_worker, args);
    return val_future(fut);
}

typedef struct {
    SkyFuture* master_future;
    Value futures_list;
} AsyncAllArgs;

static void* __async_all_worker(void* raw_args) {
    AsyncAllArgs* args = (AsyncAllArgs*)raw_args;
    Value fl = args->futures_list;
    Value result_list = val_list();
    ObjList* res_l = as_list(result_list);

    if (fl.type == VAL_OBJ && fl.as.obj != NULL) {
        if (fl.as.obj->type == OBJ_LIST) {
            ObjList* l = (ObjList*)fl.as.obj;
            for (size_t i = 0; i < l->count; i++) {
                Value res = val_future_await(l->items[i]);
                list_push(res_l, res);
            }
        } else if (fl.as.obj->type == OBJ_ARRAY) {
            ObjArray* a = (ObjArray*)fl.as.obj;
            for (size_t i = 0; i < a->capacity; i++) {
                Value res = val_future_await(a->items[i]);
                list_push(res_l, res);
            }
        } else if (fl.as.obj->type == OBJ_TUPLE) {
            ObjTuple* t = (ObjTuple*)fl.as.obj;
            for (size_t i = 0; i < t->count; i++) {
                Value res = val_future_await(t->items[i]);
                list_push(res_l, res);
            }
        }
    }
    sky_future_resolve(args->master_future, result_list);
    return NULL;
}

Value sky_async_all(int argc, Value* argv) {
    Value list_val = (argc >= 1) ? argv[0] : val_list();
    SkyFuture* fut = sky_future_create();
    fut->has_thread = true;
    AsyncAllArgs* args = (AsyncAllArgs*)sky_alloc(sizeof(AsyncAllArgs));
    args->master_future = fut;
    args->futures_list = list_val;
    pthread_create(&fut->thread, NULL, __async_all_worker, args);
    return val_future(fut);
}

typedef struct {
    SkyFuture* master_future;
    Value futures_list;
} AsyncRaceArgs;

static void* __async_race_worker(void* raw_args) {
    AsyncRaceArgs* args = (AsyncRaceArgs*)raw_args;
    Value fl = args->futures_list;

    size_t count = 0;
    Value* items = NULL;
    if (fl.type == VAL_OBJ && fl.as.obj != NULL) {
        if (fl.as.obj->type == OBJ_LIST) {
            ObjList* l = (ObjList*)fl.as.obj;
            count = l->count;
            items = l->items;
        } else if (fl.as.obj->type == OBJ_ARRAY) {
            ObjArray* a = (ObjArray*)fl.as.obj;
            count = a->capacity;
            items = a->items;
        } else if (fl.as.obj->type == OBJ_TUPLE) {
            ObjTuple* t = (ObjTuple*)fl.as.obj;
            count = t->count;
            items = t->items;
        }
    }

    if (count == 0 || !items) {
        sky_future_resolve(args->master_future, val_nil());
        return NULL;
    }

    for (;;) {
        for (size_t i = 0; i < count; i++) {
            ObjFuture* of = as_future(items[i]);
            if (of && of->future) {
                pthread_mutex_lock(&of->future->mutex);
                FutureState st = of->future->state;
                Value res = of->future->result;
                pthread_mutex_unlock(&of->future->mutex);
                if (st == FUTURE_RESOLVED) {
                    sky_future_resolve(args->master_future, res);
                    return NULL;
                }
            } else {
                sky_future_resolve(args->master_future, items[i]);
                return NULL;
            }
        }
#if defined(SKY_OS_WINDOWS)
        Sleep(1);
#else
        usleep(1000);
#endif
    }
}

Value sky_async_race(int argc, Value* argv) {
    Value list_val = (argc >= 1) ? argv[0] : val_list();
    SkyFuture* fut = sky_future_create();
    fut->has_thread = true;
    AsyncRaceArgs* args = (AsyncRaceArgs*)sky_alloc(sizeof(AsyncRaceArgs));
    args->master_future = fut;
    args->futures_list = list_val;
    pthread_create(&fut->thread, NULL, __async_race_worker, args);
    return val_future(fut);
}

typedef struct {
    SkyFuture* future;
    Value callee;
    int argc;
    Value* argv;
} AsyncSpawnArgs;

static void* __async_spawn_worker(void* raw_args) {
    AsyncSpawnArgs* args = (AsyncSpawnArgs*)raw_args;
    Value res = val_call(args->callee, args->argc, args->argv);
    sky_future_resolve(args->future, res);
    return NULL;
}

Value sky_async_spawn(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    Value callee = argv[0];
    int fn_argc = argc - 1;
    Value* fn_argv = (fn_argc > 0) ? (Value*)sky_alloc(sizeof(Value) * fn_argc) : NULL;
    for (int i = 0; i < fn_argc; i++) {
        fn_argv[i] = argv[i + 1];
    }

    SkyFuture* fut = sky_future_create();
    fut->has_thread = true;
    AsyncSpawnArgs* args = (AsyncSpawnArgs*)sky_alloc(sizeof(AsyncSpawnArgs));
    args->future = fut;
    args->callee = callee;
    args->argc = fn_argc;
    args->argv = fn_argv;
    pthread_create(&fut->thread, NULL, __async_spawn_worker, args);
    return val_future(fut);
}

const char* sky_type_name(SkyTypeId t) {
    switch (t) {
        case TYPE_NIL: return "none";
        case TYPE_INT: return "int";
        case TYPE_DOUBLE: return "double";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_STRING: return "string";
        case TYPE_ARRAY: return "array";
        case TYPE_LIST: return "list";
        case TYPE_TUPLE: return "tuple";
        case TYPE_DICT: return "dict";
        case TYPE_SET: return "set";
        case TYPE_SORTED_LIST: return "sortedList";
        case TYPE_CLASS: return "class";
        case TYPE_INSTANCE: return "instance";
        case TYPE_FUNCTION: return "function";
        case TYPE_ERROR: return "error";
        case TYPE_FUTURE: return "future";
        case TYPE_TYPE: return "type";
        default: return "unknown";
    }
}

Value val_get_type(Value v) {
    switch (v.type) {
        case VAL_NIL: return val_type(TYPE_NIL);
        case VAL_INT: return val_type(TYPE_INT);
        case VAL_DOUBLE: return val_type(TYPE_DOUBLE);
        case VAL_BOOL: return val_type(TYPE_BOOL);
        case VAL_CHAR: return val_type(TYPE_CHAR);
        case VAL_TYPE: return val_type(TYPE_TYPE);
        case VAL_OBJ:
            switch (v.as.obj->type) {
                case OBJ_STRING: return val_type(TYPE_STRING);
                case OBJ_ARRAY: return val_type(TYPE_ARRAY);
                case OBJ_LIST: return val_type(TYPE_LIST);
                case OBJ_TUPLE: return val_type(TYPE_TUPLE);
                case OBJ_DICT: return val_type(TYPE_DICT);
                case OBJ_SET: return val_type(TYPE_SET);
                case OBJ_SORTED_LIST: return val_type(TYPE_SORTED_LIST);
                case OBJ_CLASS: return val_type(TYPE_CLASS);
                case OBJ_INSTANCE: return val_type(TYPE_INSTANCE);
                case OBJ_FUNCTION: return val_type(TYPE_FUNCTION);
                case OBJ_ERROR: return val_type(TYPE_ERROR);
                case OBJ_FOREIGN: return val_type(TYPE_INSTANCE);
                case OBJ_FUTURE: return val_type(TYPE_FUTURE);
            }
    }
    return val_type(TYPE_NIL);
}

char* val_to_string(Value v) {
    char buffer[256];
    switch (v.type) {
        case VAL_NIL:
            return strdup("none");
        case VAL_INT:
            snprintf(buffer, sizeof(buffer), "%lld", (long long)v.as.i);
            return strdup(buffer);
        case VAL_DOUBLE:
            snprintf(buffer, sizeof(buffer), "%g", v.as.d);
            return strdup(buffer);
        case VAL_BOOL:
            return strdup(v.as.b ? "true" : "false");
        case VAL_CHAR:
            snprintf(buffer, sizeof(buffer), "%c", v.as.c);
            return strdup(buffer);
        case VAL_TYPE:
            snprintf(buffer, sizeof(buffer), "<type %s>", sky_type_name(v.as.t));
            return strdup(buffer);
        case VAL_OBJ: {
            switch (v.as.obj->type) {
                case OBJ_STRING: {
                    ObjString* s = (ObjString*)v.as.obj;
                    return strdup(s->chars);
                }
                case OBJ_ARRAY: {
                    ObjArray* a = (ObjArray*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "<");
                    for (size_t i = 0; i < a->capacity; ++i) {
                        char* item_s = val_to_string(a->items[i]);
                        size_t needed = strlen(res) + strlen(item_s) + 4;
                        if (needed > cap) {
                            cap = needed * 2;
                            res = (char*)GC_REALLOC(res, cap);
                        }
                        strcat(res, item_s);
                        if (i + 1 < a->capacity) strcat(res, ", ");
                        free(item_s);
                    }
                    strcat(res, ">");
                    return res;
                }
                case OBJ_LIST: {
                    ObjList* l = (ObjList*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "[");
                    for (size_t i = 0; i < l->count; ++i) {
                        char* item_s = val_to_string(l->items[i]);
                        size_t needed = strlen(res) + strlen(item_s) + 4;
                        if (needed > cap) {
                            cap = needed * 2;
                            res = (char*)GC_REALLOC(res, cap);
                        }
                        strcat(res, item_s);
                        if (i + 1 < l->count) strcat(res, ", ");
                        free(item_s);
                    }
                    strcat(res, "]");
                    return res;
                }
                case OBJ_TUPLE: {
                    ObjTuple* t = (ObjTuple*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "(");
                    for (size_t i = 0; i < t->count; ++i) {
                        char* item_s = val_to_string(t->items[i]);
                        size_t needed = strlen(res) + strlen(item_s) + 4;
                        if (needed > cap) {
                            cap = needed * 2;
                            res = (char*)GC_REALLOC(res, cap);
                        }
                        strcat(res, item_s);
                        if (i + 1 < t->count) strcat(res, ", ");
                        free(item_s);
                    }
                    strcat(res, ")");
                    return res;
                }
                case OBJ_DICT: {
                    ObjDict* d = (ObjDict*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "{");
                    size_t written = 0;
                    for (size_t i = 0; i < d->capacity; ++i) {
                        if (d->entries[i].occupied) {
                            char* k_s = val_to_string(d->entries[i].key);
                            char* v_s = val_to_string(d->entries[i].value);
                            size_t needed = strlen(res) + strlen(k_s) + strlen(v_s) + 6;
                            if (needed > cap) {
                                cap = needed * 2;
                                res = (char*)GC_REALLOC(res, cap);
                            }
                            if (written > 0) strcat(res, ", ");
                            strcat(res, k_s);
                            strcat(res, ": ");
                            strcat(res, v_s);
                            free(k_s);
                            free(v_s);
                            written++;
                        }
                    }
                    strcat(res, "}");
                    return res;
                }
                case OBJ_SET: {
                    ObjSet* s = (ObjSet*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "{");
                    size_t written = 0;
                    for (size_t i = 0; i < s->capacity; ++i) {
                        if (s->entries[i].occupied) {
                            char* item_s = val_to_string(s->entries[i].item);
                            size_t needed = strlen(res) + strlen(item_s) + 4;
                            if (needed > cap) {
                                cap = needed * 2;
                                res = (char*)GC_REALLOC(res, cap);
                            }
                            if (written > 0) strcat(res, ", ");
                            strcat(res, item_s);
                            free(item_s);
                            written++;
                        }
                    }
                    strcat(res, "}");
                    return res;
                }
                case OBJ_SORTED_LIST: {
                    ObjSortedList* sl = (ObjSortedList*)v.as.obj;
                    size_t cap = 64;
                    char* res = (char*)sky_alloc(cap);
                    strcpy(res, "SL[");
                    for (size_t i = 0; i < sl->count; ++i) {
                        char* item_s = val_to_string(sl->items[i]);
                        size_t needed = strlen(res) + strlen(item_s) + 4;
                        if (needed > cap) {
                            cap = needed * 2;
                            res = (char*)GC_REALLOC(res, cap);
                        }
                        strcat(res, item_s);
                        if (i + 1 < sl->count) strcat(res, ", ");
                        free(item_s);
                    }
                    strcat(res, "]");
                    return res;
                }
                case OBJ_CLASS: {
                    ObjClass* c = (ObjClass*)v.as.obj;
                    snprintf(buffer, sizeof(buffer), "<class %s>", c->name);
                    return strdup(buffer);
                }
                case OBJ_INSTANCE: {
                    ObjInstance* inst = (ObjInstance*)v.as.obj;
                    snprintf(buffer, sizeof(buffer), "<instance of %s>", inst->klass->name);
                    return strdup(buffer);
                }
                case OBJ_FUNCTION: {
                    ObjFunction* fn = (ObjFunction*)v.as.obj;
                    snprintf(buffer, sizeof(buffer), "<function %s>", fn->name ? fn->name : "anonymous");
                    return strdup(buffer);
                }
                case OBJ_ERROR: {
                    ObjError* err = (ObjError*)v.as.obj;
                    return strdup(err->message ? err->message : "error");
                }
                case OBJ_FOREIGN: {
                    ObjForeign* f = (ObjForeign*)v.as.obj;
                    const char* lang_str = "foreign";
                    switch (f->lang) {
                        case FOREIGN_PYTHON: lang_str = "python"; break;
                        case FOREIGN_JS: lang_str = "js"; break;
                        case FOREIGN_CPP: lang_str = "cpp"; break;
                        case FOREIGN_JAVA: lang_str = "java"; break;
                        case FOREIGN_GO: lang_str = "go"; break;
                        case FOREIGN_RUST: lang_str = "rust"; break;
                    }
                    snprintf(buffer, sizeof(buffer), "<%s object %s>", lang_str, f->name ? f->name : "");
                    return strdup(buffer);
                }
                case OBJ_FUTURE: {
                    ObjFuture* of = (ObjFuture*)v.as.obj;
                    const char* st = "pending";
                    if (of && of->future) {
                        switch (of->future->state) {
                            case FUTURE_PENDING: st = "pending"; break;
                            case FUTURE_RUNNING: st = "running"; break;
                            case FUTURE_RESOLVED: st = "resolved"; break;
                            case FUTURE_REJECTED: st = "rejected"; break;
                            case FUTURE_CANCELLED: st = "cancelled"; break;
                        }
                    }
                    snprintf(buffer, sizeof(buffer), "<Future state=%s>", st);
                    return strdup(buffer);
                }
            }
        }
    }
    return strdup("none");
}

void val_print(Value v) {
    char* s = val_to_string(v);
    fputs(s, stdout);
    if (v.type != VAL_OBJ || v.as.obj->type == OBJ_STRING || v.as.obj->type == OBJ_CLASS || v.as.obj->type == OBJ_INSTANCE || v.as.obj->type == OBJ_FUNCTION) {
        free(s);
    }
}

void val_println(Value v) {
    val_print(v);
    putchar('\n');
}

bool val_is_truthy(Value v) {
    switch (v.type) {
        case VAL_NIL: return false;
        case VAL_BOOL: return v.as.b;
        case VAL_INT: return v.as.i != 0;
        case VAL_DOUBLE: return v.as.d != 0.0;
        case VAL_CHAR: return v.as.c != '\0';
        case VAL_OBJ:
            if (v.as.obj->type == OBJ_STRING) {
                return ((ObjString*)v.as.obj)->length > 0;
            }
            if (v.as.obj->type == OBJ_LIST) {
                return ((ObjList*)v.as.obj)->count > 0;
            }
            return true;
        default: return true;
    }
}

Value val_eq(Value a, Value b) {
    if (a.type != b.type) {
        if ((a.type == VAL_INT && b.type == VAL_DOUBLE) || (a.type == VAL_DOUBLE && b.type == VAL_INT)) {
            double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
            double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
            return val_bool(da == db);
        }
        if (a.type == VAL_TYPE && b.type == VAL_OBJ && b.as.obj->type == OBJ_STRING) {
            ObjString* s = (ObjString*)b.as.obj;
            return val_bool(strcmp(sky_type_name(a.as.t), s->chars) == 0);
        }
        if (b.type == VAL_TYPE && a.type == VAL_OBJ && a.as.obj->type == OBJ_STRING) {
            ObjString* s = (ObjString*)a.as.obj;
            return val_bool(strcmp(sky_type_name(b.as.t), s->chars) == 0);
        }
        return val_bool(false);
    }
    switch (a.type) {
        case VAL_NIL: return val_bool(true);
        case VAL_INT: return val_bool(a.as.i == b.as.i);
        case VAL_DOUBLE: return val_bool(a.as.d == b.as.d);
        case VAL_BOOL: return val_bool(a.as.b == b.as.b);
        case VAL_CHAR: return val_bool(a.as.c == b.as.c);
        case VAL_TYPE: return val_bool(a.as.t == b.as.t);
        case VAL_OBJ: {
            if (a.as.obj == b.as.obj) return val_bool(true);
            if (a.as.obj->type != b.as.obj->type) return val_bool(false);
            if (a.as.obj->type == OBJ_STRING) {
                ObjString* sa = (ObjString*)a.as.obj;
                ObjString* sb = (ObjString*)b.as.obj;
                if (sa->length != sb->length) return val_bool(false);
                return val_bool(strcmp(sa->chars, sb->chars) == 0);
            }
            return val_bool(a.as.obj == b.as.obj);
        }
    }
    return val_bool(false);
}

Value val_neq(Value a, Value b) {
    Value eq = val_eq(a, b);
    return val_bool(!eq.as.b);
}

static int val_compare(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        if (a.as.i < b.as.i) return -1;
        if (a.as.i > b.as.i) return 1;
        return 0;
    }
    if ((a.type == VAL_INT || a.type == VAL_DOUBLE) && (b.type == VAL_INT || b.type == VAL_DOUBLE)) {
        double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
        double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    }
    if (a.type == VAL_CHAR && b.type == VAL_CHAR) {
        return (int)a.as.c - (int)b.as.c;
    }
    if (a.type == VAL_OBJ && b.type == VAL_OBJ && a.as.obj->type == OBJ_STRING && b.as.obj->type == OBJ_STRING) {
        return strcmp(((ObjString*)a.as.obj)->chars, ((ObjString*)b.as.obj)->chars);
    }
    return 0;
}

Value val_lt(Value a, Value b) {
    return val_bool(val_compare(a, b) < 0);
}
Value val_lte(Value a, Value b) {
    return val_bool(val_compare(a, b) <= 0);
}
Value val_gt(Value a, Value b) {
    return val_bool(val_compare(a, b) > 0);
}
Value val_gte(Value a, Value b) {
    return val_bool(val_compare(a, b) >= 0);
}

Value val_add(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        return val_int(a.as.i + b.as.i);
    }
    if ((a.type == VAL_INT || a.type == VAL_DOUBLE) && (b.type == VAL_INT || b.type == VAL_DOUBLE)) {
        double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
        double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
        return val_double(da + db);
    }

    if ((a.type == VAL_OBJ && a.as.obj->type == OBJ_STRING) || (b.type == VAL_OBJ && b.as.obj->type == OBJ_STRING)) {
        char* sa = val_to_string(a);
        char* sb = val_to_string(b);
        size_t la = strlen(sa);
        size_t lb = strlen(sb);
        char* combined = (char*)sky_alloc(la + lb + 1);
        memcpy(combined, sa, la);
        memcpy(combined + la, sb, lb);
        combined[la + lb] = '\0';
        Value res = val_string_len(combined, la + lb);
        if (a.type != VAL_OBJ || a.as.obj->type != OBJ_STRING) free(sa);
        if (b.type != VAL_OBJ || b.as.obj->type != OBJ_STRING) free(sb);
        return res;
    }

    if (a.type == VAL_OBJ && a.as.obj->type == OBJ_LIST && b.type == VAL_OBJ && b.as.obj->type == OBJ_LIST) {
        ObjList* la = (ObjList*)a.as.obj;
        ObjList* lb = (ObjList*)b.as.obj;
        Value res_v = val_list();
        ObjList* lr = (ObjList*)res_v.as.obj;
        for (size_t i = 0; i < la->count; ++i) list_push(lr, la->items[i]);
        for (size_t i = 0; i < lb->count; ++i) list_push(lr, lb->items[i]);
        return res_v;
    }
    sky_runtime_error("TypeError", "unsupported operand type(s) for +: '%s' and '%s'",
                      sky_type_name(val_get_type(a).as.t), sky_type_name(val_get_type(b).as.t));
    return val_nil();
}

Value val_sub(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) return val_int(a.as.i - b.as.i);
    if ((a.type == VAL_INT || a.type == VAL_DOUBLE) && (b.type == VAL_INT || b.type == VAL_DOUBLE)) {
        double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
        double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
        return val_double(da - db);
    }
    sky_runtime_error("TypeError", "unsupported operand type(s) for -: '%s' and '%s'",
                      sky_type_name(val_get_type(a).as.t), sky_type_name(val_get_type(b).as.t));
    return val_nil();
}

Value val_mul(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) return val_int(a.as.i * b.as.i);
    if ((a.type == VAL_INT || a.type == VAL_DOUBLE) && (b.type == VAL_INT || b.type == VAL_DOUBLE)) {
        double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
        double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
        return val_double(da * db);
    }

    if (a.type == VAL_OBJ && a.as.obj->type == OBJ_STRING && b.type == VAL_INT) {
        ObjString* s = (ObjString*)a.as.obj;
        int64_t times = b.as.i;
        if (times <= 0) return val_string("");
        size_t new_len = s->length * (size_t)times;
        char* buf = (char*)sky_alloc(new_len + 1);
        buf[0] = '\0';
        for (int64_t i = 0; i < times; ++i) {
            strcat(buf, s->chars);
        }
        return val_string_len(buf, new_len);
    }
    sky_runtime_error("TypeError", "unsupported operand type(s) for *: '%s' and '%s'",
                      sky_type_name(val_get_type(a).as.t), sky_type_name(val_get_type(b).as.t));
    return val_nil();
}

Value val_div(Value a, Value b) {
    double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
    double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
    if (db == 0.0) {
        sky_runtime_error("ZeroDivisionError", "division by zero");
        return val_double(0.0);
    }
    return val_double(da / db);
}

Value val_floordiv(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        if (b.as.i == 0) {
            sky_runtime_error("ZeroDivisionError", "integer division or modulo by zero");
            return val_int(0);
        }
        return val_int(a.as.i / b.as.i);
    }
    double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
    double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
    if (db == 0.0) {
        sky_runtime_error("ZeroDivisionError", "floor division by zero");
        return val_int(0);
    }
    return val_int((int64_t)floor(da / db));
}

Value val_pow(Value a, Value b) {
    double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
    double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
    double res = pow(da, db);
    if (a.type == VAL_INT && b.type == VAL_INT && b.as.i >= 0) {
        return val_int((int64_t)res);
    }
    return val_double(res);
}

Value val_mod(Value a, Value b) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        if (b.as.i == 0) {
            sky_runtime_error("ZeroDivisionError", "integer modulo by zero");
            return val_int(0);
        }
        return val_int(a.as.i % b.as.i);
    }
    double da = (a.type == VAL_INT) ? (double)a.as.i : a.as.d;
    double db = (b.type == VAL_INT) ? (double)b.as.i : b.as.d;
    if (db == 0.0) {
        sky_runtime_error("ZeroDivisionError", "float modulo by zero");
        return val_double(0.0);
    }
    return val_double(fmod(da, db));
}

Value val_neg(Value a) {
    if (a.type == VAL_INT) return val_int(-a.as.i);
    if (a.type == VAL_DOUBLE) return val_double(-a.as.d);
    sky_runtime_error("TypeError", "bad operand type for unary -: '%s'", sky_type_name(val_get_type(a).as.t));
    return val_nil();
}

Value val_not(Value a) {
    return val_bool(!val_is_truthy(a));
}

Value list_push(ObjList* list, Value item) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = (Value*)GC_REALLOC(list->items, sizeof(Value) * list->capacity);
    }
    list->items[list->count++] = item;
    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)list;
    return v;
}

Value list_pop(ObjList* list, int index_or_neg1) {
    if (list->count == 0) {
        sky_runtime_error("IndexError", "pop from empty list");
        return val_nil();
    }
    int idx = (index_or_neg1 == -1) ? (int)list->count - 1 : index_or_neg1;
    if (idx < 0 || (size_t)idx >= list->count) {
        sky_runtime_error("IndexError", "pop index out of range");
        return val_nil();
    }
    Value popped = list->items[idx];
    for (size_t i = (size_t)idx; i + 1 < list->count; ++i) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;
    return popped;
}

Value list_clear(ObjList* list) {
    list->count = 0;
    return val_nil();
}

Value list_size(ObjList* list) {
    return val_int((int64_t)list->count);
}

Value sorted_list_push(ObjSortedList* sl, Value item) {
    if (sl->count >= sl->capacity) {
        sl->capacity *= 2;
        sl->items = (Value*)GC_REALLOC(sl->items, sizeof(Value) * sl->capacity);
    }
    size_t i = sl->count;
    while (i > 0 && val_compare(sl->items[i - 1], item) > 0) {
        sl->items[i] = sl->items[i - 1];
        i--;
    }
    sl->items[i] = item;
    sl->count++;

    Value v;
    v.type = VAL_OBJ;
    v.as.obj = (Obj*)sl;
    return v;
}

Value sorted_list_pop(ObjSortedList* sl, int index_or_neg1) {
    if (sl->count == 0) {
        sky_runtime_error("IndexError", "pop from empty sorted list");
        return val_nil();
    }
    int idx = (index_or_neg1 == -1) ? (int)sl->count - 1 : index_or_neg1;
    if (idx < 0 || (size_t)idx >= sl->count) {
        sky_runtime_error("IndexError", "sorted list pop index out of range");
        return val_nil();
    }
    Value popped = sl->items[idx];
    for (size_t i = (size_t)idx; i + 1 < sl->count; ++i) {
        sl->items[i] = sl->items[i + 1];
    }
    sl->count--;
    return popped;
}

Value sorted_list_size(ObjSortedList* sl) {
    return val_int((int64_t)sl->count);
}

static uint32_t hash_value(Value v) {
    switch (v.type) {
        case VAL_INT: return (uint32_t)(v.as.i ^ (v.as.i >> 32));
        case VAL_CHAR: return (uint32_t)v.as.c;
        case VAL_BOOL: return v.as.b ? 1 : 0;
        case VAL_OBJ:
            if (v.as.obj->type == OBJ_STRING) {
                ObjString* s = (ObjString*)v.as.obj;
                uint32_t h = 2166136261u;
                for (size_t i = 0; i < s->length; ++i) {
                    h ^= (uint8_t)s->chars[i];
                    h *= 16777619;
                }
                return h;
            }
            return (uint32_t)(uintptr_t)v.as.obj;
        default: return 0;
    }
}

Value dict_set(ObjDict* d, Value key, Value value) {
    if ((d->count + 1) * 2 >= d->capacity) {
        size_t old_cap = d->capacity;
        DictEntry* old_entries = d->entries;
        d->capacity *= 2;
        d->entries = (DictEntry*)sky_alloc(sizeof(DictEntry) * d->capacity);
        for (size_t i = 0; i < d->capacity; ++i) d->entries[i].occupied = false;
        d->count = 0;
        for (size_t i = 0; i < old_cap; ++i) {
            if (old_entries[i].occupied) {
                dict_set(d, old_entries[i].key, old_entries[i].value);
            }
        }
    }

    uint32_t h = hash_value(key);
    size_t idx = h % d->capacity;
    while (d->entries[idx].occupied) {
        if (val_eq(d->entries[idx].key, key).as.b) {
            d->entries[idx].value = value;
            return value;
        }
        idx = (idx + 1) % d->capacity;
    }

    d->entries[idx].occupied = true;
    d->entries[idx].key = key;
    d->entries[idx].value = value;
    d->count++;
    return value;
}

Value dict_get(ObjDict* d, Value key) {
    uint32_t h = hash_value(key);
    size_t idx = h % d->capacity;
    size_t start = idx;
    while (d->entries[idx].occupied) {
        if (val_eq(d->entries[idx].key, key).as.b) {
            return d->entries[idx].value;
        }
        idx = (idx + 1) % d->capacity;
        if (idx == start) break;
    }
    return val_nil();
}

bool dict_has(ObjDict* d, Value key) {
    uint32_t h = hash_value(key);
    size_t idx = h % d->capacity;
    size_t start = idx;
    while (d->entries[idx].occupied) {
        if (val_eq(d->entries[idx].key, key).as.b) return true;
        idx = (idx + 1) % d->capacity;
        if (idx == start) break;
    }
    return false;
}

Value dict_keys(ObjDict* d) {
    Value list_v = val_list();
    ObjList* l = (ObjList*)list_v.as.obj;
    for (size_t i = 0; i < d->capacity; ++i) {
        if (d->entries[i].occupied) {
            list_push(l, d->entries[i].key);
        }
    }
    return list_v;
}

Value dict_values(ObjDict* d) {
    Value list_v = val_list();
    ObjList* l = (ObjList*)list_v.as.obj;
    for (size_t i = 0; i < d->capacity; ++i) {
        if (d->entries[i].occupied) {
            list_push(l, d->entries[i].value);
        }
    }
    return list_v;
}

Value set_add(ObjSet* s, Value item) {
    if ((s->count + 1) * 2 >= s->capacity) {
        size_t old_cap = s->capacity;
        SetEntry* old_entries = s->entries;
        s->capacity *= 2;
        s->entries = (SetEntry*)sky_alloc(sizeof(SetEntry) * s->capacity);
        for (size_t i = 0; i < s->capacity; ++i) s->entries[i].occupied = false;
        s->count = 0;
        for (size_t i = 0; i < old_cap; ++i) {
            if (old_entries[i].occupied) {
                set_add(s, old_entries[i].item);
            }
        }
    }

    uint32_t h = hash_value(item);
    size_t idx = h % s->capacity;
    while (s->entries[idx].occupied) {
        if (val_eq(s->entries[idx].item, item).as.b) return val_bool(false);
        idx = (idx + 1) % s->capacity;
    }
    s->entries[idx].occupied = true;
    s->entries[idx].item = item;
    s->count++;
    return val_bool(true);
}

bool set_has(ObjSet* s, Value item) {
    uint32_t h = hash_value(item);
    size_t idx = h % s->capacity;
    size_t start = idx;
    while (s->entries[idx].occupied) {
        if (val_eq(s->entries[idx].item, item).as.b) return true;
        idx = (idx + 1) % s->capacity;
        if (idx == start) break;
    }
    return false;
}

Value set_remove(ObjSet* s, Value item) {
    uint32_t h = hash_value(item);
    size_t idx = h % s->capacity;
    size_t start = idx;
    while (s->entries[idx].occupied) {
        if (val_eq(s->entries[idx].item, item).as.b) {
            s->entries[idx].occupied = false;
            s->count--;
            return val_bool(true);
        }
        idx = (idx + 1) % s->capacity;
        if (idx == start) break;
    }
    return val_bool(false);
}

Value val_get_index(Value target, Value index) {
    if (target.type == VAL_OBJ) {
        switch (target.as.obj->type) {
            case OBJ_STRING: {
                ObjString* s = (ObjString*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)s->length;
                if (idx < 0 || (size_t)idx >= s->length) {
                    sky_runtime_error("IndexError", "string index out of range");
                    return val_char('\0');
                }
                return val_char(s->chars[idx]);
            }
            case OBJ_ARRAY: {
                ObjArray* a = (ObjArray*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)a->capacity;
                if (idx < 0 || (size_t)idx >= a->capacity) {
                    sky_runtime_error("IndexError", "array index out of range");
                    return val_nil();
                }
                return a->items[idx];
            }
            case OBJ_LIST: {
                ObjList* l = (ObjList*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)l->count;
                if (idx < 0 || (size_t)idx >= l->count) {
                    sky_runtime_error("IndexError", "list index out of range");
                    return val_nil();
                }
                return l->items[idx];
            }
            case OBJ_TUPLE: {
                ObjTuple* t = (ObjTuple*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)t->count;
                if (idx < 0 || (size_t)idx >= t->count) {
                    sky_runtime_error("IndexError", "tuple index out of range");
                    return val_nil();
                }
                return t->items[idx];
            }
            case OBJ_DICT: {
                return dict_get((ObjDict*)target.as.obj, index);
            }
            case OBJ_SORTED_LIST: {
                ObjSortedList* sl = (ObjSortedList*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)sl->count;
                if (idx < 0 || (size_t)idx >= sl->count) {
                    sky_runtime_error("IndexError", "sorted list index out of range");
                    return val_nil();
                }
                return sl->items[idx];
            }
            default: break;
        }
    }
    sky_runtime_error("TypeError", "'%s' object is not subscriptable", sky_type_name(val_get_type(target).as.t));
    return val_nil();
}

Value val_set_index(Value target, Value index, Value val) {
    if (target.type == VAL_OBJ) {
        switch (target.as.obj->type) {
            case OBJ_ARRAY: {
                ObjArray* a = (ObjArray*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)a->capacity;
                if (idx < 0 || (size_t)idx >= a->capacity) {
                    sky_runtime_error("IndexError", "array assignment index out of range");
                    return val_nil();
                }
                a->items[idx] = val;
                return val;
            }
            case OBJ_LIST: {
                ObjList* l = (ObjList*)target.as.obj;
                int64_t idx = index.as.i;
                if (idx < 0) idx += (int64_t)l->count;
                if (idx < 0 || (size_t)idx >= l->count) {
                    sky_runtime_error("IndexError", "list assignment index out of range");
                    return val_nil();
                }
                l->items[idx] = val;
                return val;
            }
            case OBJ_DICT: {
                dict_set((ObjDict*)target.as.obj, index, val);
                return val;
            }
            default: break;
        }
    }
    sky_runtime_error("TypeError", "'%s' object does not support item assignment", sky_type_name(val_get_type(target).as.t));
    return val_nil();
}

Value val_slice(Value target, Value start, Value end) {
    if (target.type == VAL_OBJ) {
        if (target.as.obj->type == OBJ_STRING) {
            ObjString* s = (ObjString*)target.as.obj;
            int64_t st = start.type == VAL_INT ? start.as.i : 0;
            int64_t en = end.type == VAL_INT ? end.as.i : (int64_t)s->length;
            if (st < 0) st += (int64_t)s->length;
            if (en < 0) en += (int64_t)s->length;
            if (st < 0) st = 0;
            if ((size_t)en > s->length) en = (int64_t)s->length;
            if (st >= en) return val_string("");
            size_t sublen = (size_t)(en - st);
            return val_string_len(s->chars + st, sublen);
        }
        if (target.as.obj->type == OBJ_LIST) {
            ObjList* l = (ObjList*)target.as.obj;
            int64_t st = start.type == VAL_INT ? start.as.i : 0;
            int64_t en = end.type == VAL_INT ? end.as.i : (int64_t)l->count;
            if (st < 0) st += (int64_t)l->count;
            if (en < 0) en += (int64_t)l->count;
            if (st < 0) st = 0;
            if ((size_t)en > l->count) en = (int64_t)l->count;
            Value res_v = val_list();
            ObjList* lr = (ObjList*)res_v.as.obj;
            for (int64_t i = st; i < en; ++i) {
                list_push(lr, l->items[i]);
            }
            return res_v;
        }
    }
    sky_runtime_error("TypeError", "'%s' object is not sliceable", sky_type_name(val_get_type(target).as.t));
    return val_nil();
}

Value val_unpack(Value coll, size_t index) {
    if (coll.type == VAL_OBJ && coll.as.obj != NULL) {
        if (coll.as.obj->type == OBJ_TUPLE) {
            ObjTuple* t = (ObjTuple*)coll.as.obj;
            if (index < t->count) return t->items[index];
            return val_nil();
        }
        if (coll.as.obj->type == OBJ_ERROR) {
            ObjError* err = (ObjError*)coll.as.obj;
            if (index == 0) return val_nil();
            if (index == 1) return val_string(err->message);
            return val_nil();
        }
    }
    if (index == 0) return coll;
    return val_nil();
}

static const char* value_get_str(Value v) {
    if (v.type == VAL_OBJ && v.as.obj != NULL && v.as.obj->type == OBJ_STRING) {
        return ((ObjString*)v.as.obj)->chars;
    }
    return val_to_string(v);
}

static Value sky_str_split_impl(const char* s, const char* delim) {
    size_t dlen = strlen(delim);
    Value result = val_list();
    ObjList* list = as_list(result);
    if (dlen == 0) {
        for (const char* p = s; *p; p++) {
            char c[2] = {*p, '\0'};
            list_push(list, val_string(c));
        }
        return result;
    }
    const char* start = s;
    const char* found;
    while ((found = strstr(start, delim)) != NULL) {
        list_push(list, val_string_len(start, found - start));
        start = found + dlen;
    }
    list_push(list, val_string(start));
    return result;
}

static Value sky_str_join_impl(const char* delim, Value coll) {
    if (coll.type != VAL_OBJ || coll.as.obj == NULL) return val_string("");
    size_t count = 0;
    Value* items = NULL;
    if (coll.as.obj->type == OBJ_LIST) {
        ObjList* l = (ObjList*)coll.as.obj;
        count = l->count;
        items = l->items;
    } else if (coll.as.obj->type == OBJ_TUPLE) {
        ObjTuple* t = (ObjTuple*)coll.as.obj;
        count = t->count;
        items = t->items;
    } else if (coll.as.obj->type == OBJ_ARRAY) {
        ObjArray* a = (ObjArray*)coll.as.obj;
        count = a->capacity;
        items = a->items;
    } else {
        return val_string("");
    }
    size_t dlen = strlen(delim);
    size_t cap = 256, len = 0;
    char* buf = (char*)GC_MALLOC(cap);
    for (size_t i = 0; i < count; i++) {
        char* s = val_to_string(items[i]);
        size_t sl = strlen(s);
        while (len + sl + dlen + 1 >= cap) {
            cap *= 2;
            buf = (char*)GC_REALLOC(buf, cap);
        }
        if (i > 0) {
            memcpy(buf + len, delim, dlen);
            len += dlen;
        }
        memcpy(buf + len, s, sl);
        len += sl;
        free(s);
    }
    buf[len] = '\0';
    return val_string(buf);
}

static Value sky_str_upper_impl(const char* s) {
    size_t len = strlen(s);
    char* buf = (char*)GC_MALLOC(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = toupper((unsigned char)s[i]);
    buf[len] = '\0';
    return val_string(buf);
}

static Value sky_str_lower_impl(const char* s) {
    size_t len = strlen(s);
    char* buf = (char*)GC_MALLOC(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = tolower((unsigned char)s[i]);
    buf[len] = '\0';
    return val_string(buf);
}

static Value sky_str_trim_impl(const char* s) {
    const char* start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    const char* end = s + strlen(s) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    return val_string_len(start, end - start + 1);
}

static Value sky_str_trimleft_impl(const char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return val_string(s);
}

static Value sky_str_trimright_impl(const char* s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    return val_string_len(s, len);
}

static Value sky_str_replace_impl(const char* s, const char* old_s, const char* new_s) {
    size_t old_len = strlen(old_s), new_len = strlen(new_s);
    if (old_len == 0) return val_string(s);
    int count = 0;
    const char* p = s;
    while ((p = strstr(p, old_s)) != NULL) { count++; p += old_len; }
    if (count == 0) return val_string(s);

    size_t result_len = strlen(s) + count * ((int)new_len - (int)old_len);
    char* buf = (char*)GC_MALLOC(result_len + 1);
    char* dst = buf;
    p = s;
    const char* found;
    while ((found = strstr(p, old_s)) != NULL) {
        size_t chunk = found - p;
        memcpy(dst, p, chunk); dst += chunk;
        memcpy(dst, new_s, new_len); dst += new_len;
        p = found + old_len;
    }
    strcpy(dst, p);
    return val_string(buf);
}

static Value sky_str_reverse_impl(const char* s) {
    size_t len = strlen(s);
    char* buf = (char*)GC_MALLOC(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = s[len - 1 - i];
    buf[len] = '\0';
    return val_string(buf);
}

Value val_get_prop(Value target, const char* name) {
    if (strcmp(name, "T") == 0) {
        return val_get_type(target);
    }
    if (strcmp(name, "size") == 0 || strcmp(name, "length") == 0) {
        if (target.type == VAL_OBJ && target.as.obj != NULL) {
            switch (target.as.obj->type) {
                case OBJ_STRING: return val_int((int64_t)((ObjString*)target.as.obj)->length);
                case OBJ_ARRAY: return val_int((int64_t)((ObjArray*)target.as.obj)->capacity);
                case OBJ_LIST: return val_int((int64_t)((ObjList*)target.as.obj)->count);
                case OBJ_TUPLE: return val_int((int64_t)((ObjTuple*)target.as.obj)->count);
                case OBJ_DICT: return val_int((int64_t)((ObjDict*)target.as.obj)->count);
                case OBJ_SET: return val_int((int64_t)((ObjSet*)target.as.obj)->count);
                case OBJ_SORTED_LIST: return val_int((int64_t)((ObjSortedList*)target.as.obj)->count);
                default: break;
            }
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_STRING) {
        ObjString* s = (ObjString*)target.as.obj;
        if (strcmp(name, "chars") == 0) {
            Value result = val_list();
            ObjList* list = as_list(result);
            for (const char* p = s->chars; *p; p++) {
                char c[2] = {*p, '\0'};
                list_push(list, val_string(c));
            }
            return result;
        }
        if (strcmp(name, "bytes") == 0) {
            Value result = val_list();
            ObjList* list = as_list(result);
            for (const char* p = s->chars; *p; p++) {
                list_push(list, val_int((unsigned char)*p));
            }
            return result;
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_FUTURE) {
        ObjFuture* of = (ObjFuture*)target.as.obj;
        SkyFuture* fut = of->future;
        if (strcmp(name, "is_done") == 0) {
            bool done = (fut && fut->state != FUTURE_PENDING && fut->state != FUTURE_RUNNING);
            return val_bool(done);
        }
        if (strcmp(name, "result") == 0) {
            return fut ? fut->result : val_nil();
        }
        if (strcmp(name, "error") == 0) {
            return val_string((fut && fut->error_msg) ? fut->error_msg : "");
        }
        if (strcmp(name, "state") == 0) {
            const char* st = "pending";
            if (fut) {
                switch (fut->state) {
                    case FUTURE_PENDING: st = "pending"; break;
                    case FUTURE_RUNNING: st = "running"; break;
                    case FUTURE_RESOLVED: st = "resolved"; break;
                    case FUTURE_REJECTED: st = "rejected"; break;
                    case FUTURE_CANCELLED: st = "cancelled"; break;
                }
            }
            return val_string(st);
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_DICT) {
        ObjDict* d = (ObjDict*)target.as.obj;
        Value key = val_string(name);
        if (dict_has(d, key)) {
            return dict_get(d, key);
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_FOREIGN) {
        ObjForeign* f = (ObjForeign*)target.as.obj;
        switch (f->lang) {
            case FOREIGN_PYTHON: return sky_python_get_prop(f, name);
            case FOREIGN_JS: return sky_js_get_prop(f, name);
            case FOREIGN_CPP: return sky_cpp_get_prop(f, name);
            case FOREIGN_JAVA: return sky_java_get_prop(f, name);
            case FOREIGN_GO: return sky_go_get_prop(f, name);
            case FOREIGN_RUST: return sky_rust_get_prop(f, name);
            default: break;
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_INSTANCE) {
        ObjInstance* inst = (ObjInstance*)target.as.obj;
        FieldEntry* f = inst->fields;
        while (f) {
            if (strcmp(f->name, name) == 0) return f->value;
            f = f->next;
        }

        MethodEntry* m = inst->klass->methods;
        while (m) {
            if (strcmp(m->name, name) == 0) return m->fn;
            m = m->next;
        }
    }
    return val_nil();
}

Value val_set_prop(Value target, const char* name, Value val) {
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_DICT) {
        ObjDict* d = (ObjDict*)target.as.obj;
        dict_set(d, val_string(name), val);
        return val;
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_FOREIGN) {
        ObjForeign* f = (ObjForeign*)target.as.obj;
        switch (f->lang) {
            case FOREIGN_PYTHON: return sky_python_set_prop(f, name, val);
            case FOREIGN_JS: return sky_js_set_prop(f, name, val);
            case FOREIGN_CPP: return sky_cpp_set_prop(f, name, val);
            case FOREIGN_JAVA: return sky_java_set_prop(f, name, val);
            case FOREIGN_GO: return sky_go_set_prop(f, name, val);
            case FOREIGN_RUST: return sky_rust_set_prop(f, name, val);
            default: break;
        }
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL && target.as.obj->type == OBJ_INSTANCE) {
        ObjInstance* inst = (ObjInstance*)target.as.obj;
        FieldEntry* f = inst->fields;
        while (f) {
            if (strcmp(f->name, name) == 0) {
                f->value = val;
                return val;
            }
            f = f->next;
        }

        FieldEntry* nf = (FieldEntry*)sky_alloc(sizeof(FieldEntry));
        nf->name = name;
        nf->value = val;
        nf->next = inst->fields;
        inst->fields = nf;
        return val;
    }
    sky_runtime_error("TypeError", "cannot set property '%s' on non-instance '%s'", name, sky_type_name(val_get_type(target).as.t));
    return val_nil();
}

Value val_call_method_kw(Value target, const char* name, int argc, Value* argv, const char** arg_names) {
    if (strcmp(name, "size") == 0 || strcmp(name, "length") == 0) {
        return val_get_prop(target, "size");
    }
    if (strcmp(name, "value") == 0 && argc >= 1) {
        return val_get_index(target, argv[0]);
    }
    if (target.type == VAL_OBJ && target.as.obj != NULL) {
        switch (target.as.obj->type) {
            case OBJ_STRING: {
                ObjString* s = (ObjString*)target.as.obj;
                if (strcmp(name, "split") == 0) {
                    const char* delim = (argc >= 1) ? ((argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0])) : " ";
                    return sky_str_split_impl(s->chars, delim);
                }
                if (strcmp(name, "join") == 0 && argc >= 1) {
                    return sky_str_join_impl(s->chars, argv[0]);
                }
                if (strcmp(name, "upper") == 0) {
                    return sky_str_upper_impl(s->chars);
                }
                if (strcmp(name, "lower") == 0) {
                    return sky_str_lower_impl(s->chars);
                }
                if (strcmp(name, "trim") == 0) {
                    return sky_str_trim_impl(s->chars);
                }
                if (strcmp(name, "trimleft") == 0) {
                    return sky_str_trimleft_impl(s->chars);
                }
                if (strcmp(name, "trimright") == 0) {
                    return sky_str_trimright_impl(s->chars);
                }
                if (strcmp(name, "contains") == 0 || strcmp(name, "has") == 0) {
                    if (argc < 1) return val_bool(false);
                    const char* sub = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    return val_bool(strstr(s->chars, sub) != NULL);
                }
                if (strcmp(name, "startswith") == 0) {
                    if (argc < 1) return val_bool(false);
                    const char* prefix = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    return val_bool(strncmp(s->chars, prefix, strlen(prefix)) == 0);
                }
                if (strcmp(name, "endswith") == 0) {
                    if (argc < 1) return val_bool(false);
                    const char* suffix = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    size_t slen = s->length, xlen = strlen(suffix);
                    if (xlen > slen) return val_bool(false);
                    return val_bool(strcmp(s->chars + slen - xlen, suffix) == 0);
                }
                if (strcmp(name, "replace") == 0) {
                    if (argc < 2) return target;
                    const char* old_s = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    const char* new_s = (argv[1].type == VAL_OBJ && argv[1].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[1].as.obj)->chars : val_to_string(argv[1]);
                    return sky_str_replace_impl(s->chars, old_s, new_s);
                }
                if (strcmp(name, "find") == 0) {
                    if (argc < 1) return val_int(-1);
                    const char* sub = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    const char* found = strstr(s->chars, sub);
                    return val_int(found ? (int64_t)(found - s->chars) : -1);
                }
                if (strcmp(name, "count") == 0) {
                    if (argc < 1) return val_int(0);
                    const char* sub = (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : val_to_string(argv[0]);
                    size_t sublen = strlen(sub);
                    if (sublen == 0) return val_int(0);
                    int count = 0;
                    const char* p = s->chars;
                    while ((p = strstr(p, sub)) != NULL) { count++; p += sublen; }
                    return val_int(count);
                }
                if (strcmp(name, "reverse") == 0) {
                    return sky_str_reverse_impl(s->chars);
                }
                if (strcmp(name, "chars") == 0) {
                    return val_get_prop(target, "chars");
                }
                if (strcmp(name, "bytes") == 0) {
                    return val_get_prop(target, "bytes");
                }
                break;
            }
            case OBJ_LIST: {
                ObjList* l = (ObjList*)target.as.obj;
                if (strcmp(name, "push") == 0 && argc >= 1) {
                    return list_push(l, argv[0]);
                }
                if (strcmp(name, "pop") == 0) {
                    int idx = (argc >= 1 && argv[0].type == VAL_INT) ? (int)argv[0].as.i : -1;
                    return list_pop(l, idx);
                }
                if (strcmp(name, "clear") == 0) {
                    return list_clear(l);
                }
                if (strcmp(name, "join") == 0) {
                    const char* delim = (argc >= 1 && argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) ? ((ObjString*)argv[0].as.obj)->chars : ((argc >= 1) ? val_to_string(argv[0]) : "");
                    return sky_str_join_impl(delim, target);
                }
                if (strcmp(name, "contains") == 0 || strcmp(name, "has") == 0) {
                    if (argc < 1) return val_bool(false);
                    for (size_t i = 0; i < l->count; ++i) {
                        if (val_eq(l->items[i], argv[0]).as.b) return val_bool(true);
                    }
                    return val_bool(false);
                }
                if (strcmp(name, "find") == 0) {
                    if (argc < 1) return val_int(-1);
                    for (size_t i = 0; i < l->count; ++i) {
                        if (val_eq(l->items[i], argv[0]).as.b) return val_int((int64_t)i);
                    }
                    return val_int(-1);
                }
                if (strcmp(name, "count") == 0) {
                    if (argc < 1) return val_int(0);
                    int cnt = 0;
                    for (size_t i = 0; i < l->count; ++i) {
                        if (val_eq(l->items[i], argv[0]).as.b) cnt++;
                    }
                    return val_int(cnt);
                }
                if (strcmp(name, "reverse") == 0) {
                    Value res = val_list();
                    ObjList* rl = as_list(res);
                    for (size_t i = 0; i < l->count; ++i) {
                        list_push(rl, l->items[l->count - 1 - i]);
                    }
                    return res;
                }
                break;
            }
            case OBJ_SORTED_LIST: {
                ObjSortedList* sl = (ObjSortedList*)target.as.obj;
                if (strcmp(name, "push") == 0 && argc >= 1) {
                    return sorted_list_push(sl, argv[0]);
                }
                if (strcmp(name, "pop") == 0) {
                    int idx = (argc >= 1 && argv[0].type == VAL_INT) ? (int)argv[0].as.i : -1;
                    return sorted_list_pop(sl, idx);
                }
                break;
            }
            case OBJ_DICT: {
                ObjDict* d = (ObjDict*)target.as.obj;

                {
                    Value key = val_string(name);
                    if (dict_has(d, key)) {
                        Value fn_val = dict_get(d, key);
                        if (fn_val.type == VAL_OBJ && (fn_val.as.obj->type == OBJ_FUNCTION || fn_val.as.obj->type == OBJ_CLASS)) {
                            return val_call_kw(fn_val, argc, argv, arg_names);
                        }
                    }
                }
                if (strcmp(name, "keys") == 0) return dict_keys(d);
                if (strcmp(name, "values") == 0) return dict_values(d);
                if (strcmp(name, "has") == 0 && argc >= 1) return val_bool(dict_has(d, argv[0]));
                break;
            }
            case OBJ_SET: {
                ObjSet* s = (ObjSet*)target.as.obj;
                if (strcmp(name, "add") == 0 && argc >= 1) return set_add(s, argv[0]);
                if (strcmp(name, "has") == 0 && argc >= 1) return val_bool(set_has(s, argv[0]));
                if (strcmp(name, "remove") == 0 && argc >= 1) return set_remove(s, argv[0]);
                break;
            }
            case OBJ_INSTANCE: {
                ObjInstance* inst = (ObjInstance*)target.as.obj;
                MethodEntry* m = inst->klass->methods;
                while (m) {
                    if (strcmp(m->name, name) == 0) {
                        ObjFunction* mfn = (m->fn.type == VAL_OBJ && m->fn.as.obj->type == OBJ_FUNCTION)
                                           ? (ObjFunction*)m->fn.as.obj : NULL;
                        if (!mfn) {
                            Value* method_args = (Value*)sky_alloc(sizeof(Value) * (argc + 1));
                            method_args[0] = target;
                            for (int i = 0; i < argc; ++i) method_args[i + 1] = argv[i];
                            return val_call(m->fn, argc + 1, method_args);
                        }

                        if (mfn->param_count == 0) {
                            int total = argc + 1;
                            Value* method_args = (Value*)sky_alloc(sizeof(Value) * total);
                            method_args[0] = target;
                            for (int i = 0; i < argc; ++i) method_args[i + 1] = argv[i];
                            return mfn->fn(total, method_args);
                        }

                        int total_params = mfn->param_count;
                        int total_args = 1 + (argc > total_params ? argc : total_params);
                        Value* method_args = (Value*)sky_alloc(sizeof(Value) * total_args);
                        bool* param_set = (bool*)sky_alloc(sizeof(bool) * total_params);
                        method_args[0] = target;
                        for (int i = 1; i < total_args; ++i) method_args[i] = val_nil();
                        for (int i = 0; i < total_params; ++i) param_set[i] = false;

                        int next_pos = 0;
                        for (int i = 0; i < argc; ++i) {
                            if (arg_names != NULL && arg_names[i] != NULL) {
                                int matched = -1;
                                for (int p = 0; p < total_params; ++p) {
                                    if (strcmp(mfn->param_names[p], arg_names[i]) == 0) {
                                        matched = p;
                                        break;
                                    }
                                }
                                if (matched < 0) {
                                    sky_runtime_error("PermissionError", "Method '%s' gateway rejected variable '%s' (not in takes)",
                                                      name, arg_names[i]);
                                    return val_nil();
                                }
                                method_args[1 + matched] = argv[i];
                                param_set[matched] = true;
                            } else {
                                while (next_pos < total_params && param_set[next_pos]) {
                                    next_pos++;
                                }
                                if (next_pos < total_params) {
                                    method_args[1 + next_pos] = argv[i];
                                    param_set[next_pos] = true;
                                    next_pos++;
                                } else if (1 + i < total_args) {
                                    method_args[1 + i] = argv[i];
                                }
                            }
                        }
                        return mfn->fn(total_args, method_args);
                    }
                    m = m->next;
                }
                break;
            }
            case OBJ_FOREIGN: {
                ObjForeign* f = (ObjForeign*)target.as.obj;
                switch (f->lang) {
                    case FOREIGN_PYTHON: return sky_python_call_method(f, name, argc, argv);
                    case FOREIGN_JS: return sky_js_call_method(f, name, argc, argv);
                    case FOREIGN_CPP: return sky_cpp_call_method(f, name, argc, argv);
                    case FOREIGN_JAVA: return sky_java_call_method(f, name, argc, argv);
                    case FOREIGN_GO: return sky_go_call_method(f, name, argc, argv);
                    case FOREIGN_RUST: return sky_rust_call_method(f, name, argc, argv);
                    default: break;
                }
                break;
            }
            case OBJ_FUTURE: {
                ObjFuture* of = (ObjFuture*)target.as.obj;
                if (strcmp(name, "await") == 0) {
                    return sky_future_await(of->future);
                }
                if (strcmp(name, "cancel") == 0) {
                    if (of->future) {
                        pthread_mutex_lock(&of->future->mutex);
                        of->future->state = FUTURE_CANCELLED;
                        pthread_cond_broadcast(&of->future->cond);
                        pthread_mutex_unlock(&of->future->mutex);
                    }
                    return val_bool(true);
                }
                break;
            }
            default: break;
        }
    }
    sky_runtime_error("AttributeError", "'%s' object has no attribute '%s'", sky_type_name(val_get_type(target).as.t), name);
    return val_nil();
}

Value val_call_method(Value target, const char* name, int argc, Value* argv) {
    return val_call_method_kw(target, name, argc, argv, NULL);
}

Value val_call_kw(Value callee, int argc, Value* argv, const char** arg_names) {
    if (callee.type == VAL_OBJ) {
        if (callee.as.obj->type == OBJ_FUNCTION) {
            ObjFunction* fn = (ObjFunction*)callee.as.obj;
            if (fn->param_count == 0) {
                return fn->fn(argc, argv);
            }

            int total_args = argc > fn->param_count ? argc : fn->param_count;
            Value* call_argv = (Value*)sky_alloc(sizeof(Value) * (total_args > 0 ? total_args : 1));
            bool* param_set = (bool*)sky_alloc(sizeof(bool) * (fn->param_count > 0 ? fn->param_count : 1));
            for (int i = 0; i < total_args; ++i) call_argv[i] = val_nil();
            for (int i = 0; i < fn->param_count; ++i) param_set[i] = false;

            int next_pos = 0;
            for (int i = 0; i < argc; ++i) {
                if (arg_names != NULL && arg_names[i] != NULL) {

                    int matched = -1;
                    for (int p = 0; p < fn->param_count; ++p) {
                        if (strcmp(fn->param_names[p], arg_names[i]) == 0) {
                            matched = p;
                            break;
                        }
                    }
                    if (matched < 0) {
                        sky_runtime_error("PermissionError", "'%s' gateway rejected variable '%s' (not in takes)",
                                          fn->name ? fn->name : "function", arg_names[i]);
                        return val_nil();
                    }
                    call_argv[matched] = argv[i];
                    param_set[matched] = true;
                } else {
                    while (next_pos < fn->param_count && param_set[next_pos]) {
                        next_pos++;
                    }
                    if (next_pos < fn->param_count) {
                        call_argv[next_pos] = argv[i];
                        param_set[next_pos] = true;
                        next_pos++;
                    } else if (i < total_args) {
                        call_argv[i] = argv[i];
                    }
                }
            }
            return fn->fn(total_args, call_argv);
        }
        if (callee.as.obj->type == OBJ_CLASS) {
            ObjClass* k = (ObjClass*)callee.as.obj;
            Value inst_v = val_instance(k);
            MethodEntry* m = k->methods;
            while (m) {
                if (strcmp(m->name, "init") == 0) {
                    val_call_method_kw(inst_v, "init", argc, argv, arg_names);
                    break;
                }
                m = m->next;
            }
            return inst_v;
        }
    }
    sky_runtime_error("TypeError", "'%s' object is not callable", sky_type_name(val_get_type(callee).as.t));
    return val_nil();
}

Value val_call(Value callee, int argc, Value* argv) {
    return val_call_kw(callee, argc, argv, NULL);
}

void sky_check_takes(const char* fn_name, int actual_argc, int expected_argc, ...) {
    if (actual_argc < expected_argc) {
        sky_runtime_error("TypeError", "%s() expected at least %d arguments but got %d",
                          fn_name ? fn_name : "function", expected_argc, actual_argc);
    }
}

Value sky_builtin_split(int argc, Value* argv) {
    if (argc < 1) return val_list();
    const char* s = value_get_str(argv[0]);
    const char* delim = (argc >= 2) ? value_get_str(argv[1]) : " ";
    return sky_str_split_impl(s, delim);
}

Value sky_builtin_join(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    if (argc == 1) {
        return sky_str_join_impl("", argv[0]);
    }
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL &&
        (argv[0].as.obj->type == OBJ_LIST || argv[0].as.obj->type == OBJ_TUPLE || argv[0].as.obj->type == OBJ_ARRAY)) {
        const char* delim = value_get_str(argv[1]);
        return sky_str_join_impl(delim, argv[0]);
    }
    if (argv[1].type == VAL_OBJ && argv[1].as.obj != NULL &&
        (argv[1].as.obj->type == OBJ_LIST || argv[1].as.obj->type == OBJ_TUPLE || argv[1].as.obj->type == OBJ_ARRAY)) {
        const char* delim = value_get_str(argv[0]);
        return sky_str_join_impl(delim, argv[1]);
    }
    return val_string("");
}

Value sky_builtin_upper(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    return sky_str_upper_impl(value_get_str(argv[0]));
}

Value sky_builtin_lower(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    return sky_str_lower_impl(value_get_str(argv[0]));
}

Value sky_builtin_trim(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    return sky_str_trim_impl(value_get_str(argv[0]));
}

Value sky_builtin_trimleft(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    return sky_str_trimleft_impl(value_get_str(argv[0]));
}

Value sky_builtin_trimright(int argc, Value* argv) {
    if (argc < 1) return val_string("");
    return sky_str_trimright_impl(value_get_str(argv[0]));
}

Value sky_builtin_contains(int argc, Value* argv) {
    if (argc < 2) return val_bool(false);
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL) {
        if (argv[0].as.obj->type == OBJ_STRING) {
            return val_bool(strstr(((ObjString*)argv[0].as.obj)->chars, value_get_str(argv[1])) != NULL);
        }
        if (argv[0].as.obj->type == OBJ_LIST) {
            ObjList* l = (ObjList*)argv[0].as.obj;
            for (size_t i = 0; i < l->count; ++i) {
                if (val_eq(l->items[i], argv[1]).as.b) return val_bool(true);
            }
            return val_bool(false);
        }
        if (argv[0].as.obj->type == OBJ_DICT) {
            return val_bool(dict_has((ObjDict*)argv[0].as.obj, argv[1]));
        }
        if (argv[0].as.obj->type == OBJ_SET) {
            return val_bool(set_has((ObjSet*)argv[0].as.obj, argv[1]));
        }
    }
    return val_bool(false);
}

Value sky_builtin_startswith(int argc, Value* argv) {
    if (argc < 2) return val_bool(false);
    const char* s = value_get_str(argv[0]);
    const char* prefix = value_get_str(argv[1]);
    return val_bool(strncmp(s, prefix, strlen(prefix)) == 0);
}

Value sky_builtin_endswith(int argc, Value* argv) {
    if (argc < 2) return val_bool(false);
    const char* s = value_get_str(argv[0]);
    const char* suffix = value_get_str(argv[1]);
    size_t slen = strlen(s), xlen = strlen(suffix);
    if (xlen > slen) return val_bool(false);
    return val_bool(strcmp(s + slen - xlen, suffix) == 0);
}

Value sky_builtin_replace(int argc, Value* argv) {
    if (argc < 3) return (argc >= 1) ? argv[0] : val_string("");
    return sky_str_replace_impl(value_get_str(argv[0]), value_get_str(argv[1]), value_get_str(argv[2]));
}

Value sky_builtin_find(int argc, Value* argv) {
    if (argc < 2) return val_int(-1);
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL && argv[0].as.obj->type == OBJ_STRING) {
        const char* s = ((ObjString*)argv[0].as.obj)->chars;
        const char* sub = value_get_str(argv[1]);
        const char* found = strstr(s, sub);
        return val_int(found ? (int64_t)(found - s) : -1);
    }
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL && argv[0].as.obj->type == OBJ_LIST) {
        ObjList* l = (ObjList*)argv[0].as.obj;
        for (size_t i = 0; i < l->count; ++i) {
            if (val_eq(l->items[i], argv[1]).as.b) return val_int((int64_t)i);
        }
    }
    return val_int(-1);
}

Value sky_builtin_count(int argc, Value* argv) {
    if (argc < 2) return val_int(0);
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL && argv[0].as.obj->type == OBJ_STRING) {
        const char* s = ((ObjString*)argv[0].as.obj)->chars;
        const char* sub = value_get_str(argv[1]);
        size_t sublen = strlen(sub);
        if (sublen == 0) return val_int(0);
        int count = 0;
        const char* p = s;
        while ((p = strstr(p, sub)) != NULL) { count++; p += sublen; }
        return val_int(count);
    }
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL && argv[0].as.obj->type == OBJ_LIST) {
        ObjList* l = (ObjList*)argv[0].as.obj;
        int count = 0;
        for (size_t i = 0; i < l->count; ++i) {
            if (val_eq(l->items[i], argv[1]).as.b) count++;
        }
        return val_int(count);
    }
    return val_int(0);
}

Value sky_builtin_reverse(int argc, Value* argv) {
    if (argc < 1) return val_nil();
    if (argv[0].type == VAL_OBJ && argv[0].as.obj != NULL) {
        if (argv[0].as.obj->type == OBJ_STRING) {
            return sky_str_reverse_impl(((ObjString*)argv[0].as.obj)->chars);
        }
        if (argv[0].as.obj->type == OBJ_LIST) {
            ObjList* l = (ObjList*)argv[0].as.obj;
            Value res = val_list();
            ObjList* rl = as_list(res);
            for (size_t i = 0; i < l->count; ++i) {
                list_push(rl, l->items[l->count - 1 - i]);
            }
            return res;
        }
    }
    return argv[0];
}

Value sky_builtin_chars(int argc, Value* argv) {
    if (argc < 1) return val_list();
    const char* s = value_get_str(argv[0]);
    Value result = val_list();
    ObjList* list = as_list(result);
    for (const char* p = s; *p; p++) {
        char c[2] = {*p, '\0'};
        list_push(list, val_string(c));
    }
    return result;
}

Value sky_builtin_bytes(int argc, Value* argv) {
    if (argc < 1) return val_list();
    const char* s = value_get_str(argv[0]);
    Value result = val_list();
    ObjList* list = as_list(result);
    for (const char* p = s; *p; p++) {
        list_push(list, val_int((unsigned char)*p));
    }
    return result;
}

