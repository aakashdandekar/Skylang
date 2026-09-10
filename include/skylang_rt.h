#ifndef SKYLANG_RT_H
#define SKYLANG_RT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#ifndef GC_THREADS
#define GC_THREADS 1
#endif
#include <gc.h>
#include "sky_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Value representation */
typedef enum {
    VAL_NIL,
    VAL_INT,
    VAL_DOUBLE,
    VAL_BOOL,
    VAL_CHAR,
    VAL_TYPE,
    VAL_OBJ
} ValueType;

typedef enum {
    TYPE_NIL = 0,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_LIST,
    TYPE_TUPLE,
    TYPE_DICT,
    TYPE_SET,
    TYPE_SORTED_LIST,
    TYPE_CLASS,
    TYPE_INSTANCE,
    TYPE_FUNCTION,
    TYPE_ERROR,
    TYPE_FUTURE,
    TYPE_TYPE
} SkyTypeId;

typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_LIST,
    OBJ_TUPLE,
    OBJ_DICT,
    OBJ_SET,
    OBJ_SORTED_LIST,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_FUNCTION,
    OBJ_ERROR,
    OBJ_FOREIGN,
    OBJ_FUTURE
} ObjType;

typedef struct Obj {
    ObjType type;
} Obj;

typedef struct Value {
    ValueType type;
    union {
        int64_t i;
        double d;
        bool b;
        char c;
        SkyTypeId t;
        Obj* obj;
    } as;
} Value;

/* String object */
typedef struct {
    Obj header;
    size_t length;
    char* chars;
} ObjString;

/* Array object (fixed size, homogeneous element type) */
typedef struct {
    Obj header;
    size_t capacity;
    SkyTypeId elem_type;
    Value* items;
} ObjArray;

/* List object (dynamic size, heterogeneous) */
typedef struct {
    Obj header;
    size_t capacity;
    size_t count;
    Value* items;
} ObjList;

/* Tuple object (fixed size, immutable) */
typedef struct {
    Obj header;
    size_t count;
    Value* items;
} ObjTuple;

/* Dict entry and object */
typedef struct {
    Value key;
    Value value;
    bool occupied;
} DictEntry;

typedef struct {
    Obj header;
    size_t capacity;
    size_t count;
    DictEntry* entries;
} ObjDict;

/* Set entry and object */
typedef struct {
    Value item;
    bool occupied;
} SetEntry;

typedef struct {
    Obj header;
    size_t capacity;
    size_t count;
    SetEntry* entries;
} ObjSet;

/* SortedList object (always ordered) */
typedef struct {
    Obj header;
    size_t capacity;
    size_t count;
    Value* items;
} ObjSortedList;

/* Function pointer signature */
typedef Value (*SkyNativeFn)(int argc, Value* argv);

typedef struct {
    Obj header;
    const char* name;
    SkyNativeFn fn;
    int arity;
    int param_count;
    const char** param_names;
} ObjFunction;

/* Class and Instance */
typedef struct MethodEntry {
    const char* name;
    Value fn;
    struct MethodEntry* next;
} MethodEntry;

typedef struct {
    Obj header;
    const char* name;
    MethodEntry* methods;
} ObjClass;

typedef struct FieldEntry {
    const char* name;
    Value value;
    struct FieldEntry* next;
} FieldEntry;

typedef struct {
    Obj header;
    ObjClass* klass;
    FieldEntry* fields;
} ObjInstance;

/* Error Object */
typedef struct {
    Obj header;
    char* message;
} ObjError;

/* Foreign Language type */
typedef enum {
    FOREIGN_PYTHON,
    FOREIGN_JS,
    FOREIGN_CPP,
    FOREIGN_JAVA,
    FOREIGN_GO,
    FOREIGN_RUST
} ForeignLang;

/* Foreign Object (wraps Python PyObject*, JS object, C++ pointer, Java jobject) */
typedef struct {
    Obj header;
    ForeignLang lang;
    char* name;
    void* handle;
    void* extra;
} ObjForeign;

/* Future states */
typedef enum {
    FUTURE_PENDING,
    FUTURE_RUNNING,
    FUTURE_RESOLVED,
    FUTURE_REJECTED,
    FUTURE_CANCELLED
} FutureState;

/* Concurrency Future */
typedef struct SkyFuture {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FutureState state;
    Value result;
    char* error_msg;
    bool has_thread;
    bool is_joined;
} SkyFuture;

typedef struct {
    Obj header;
    SkyFuture* future;
} ObjFuture;

/* Value Constructors */
Value val_nil(void);
Value val_int(int64_t v);
Value val_double(double v);
Value val_bool(bool v);
Value val_char(char v);
Value val_type(SkyTypeId t);
Value val_string(const char* str);
Value val_string_len(const char* str, size_t len);
Value val_error(const char* message);
Value val_array(size_t capacity, SkyTypeId elem_type);
Value val_list(void);
Value val_tuple(size_t count, Value* items);
Value val_dict(void);
Value val_set(void);
Value val_sorted_list(void);
Value val_class(const char* name);
Value val_instance(ObjClass* klass);
Value val_foreign(ForeignLang lang, const char* name, void* handle, void* extra);
Value val_function(const char* name, SkyNativeFn fn, int arity);
Value val_function_with_params(const char* name, SkyNativeFn fn, int arity, int param_count, const char** param_names);
Value val_future(SkyFuture* fut);

/* Object helpers */
ObjString* as_string(Value v);
ObjArray* as_array(Value v);
ObjList* as_list(Value v);
ObjTuple* as_tuple(Value v);
ObjDict* as_dict(Value v);
ObjSet* as_set(Value v);
ObjSortedList* as_sorted_list(Value v);
ObjClass* as_class(Value v);
ObjInstance* as_instance(Value v);
ObjForeign* as_foreign(Value v);
ObjFuture* as_future(Value v);

/* Future lifecycle & async operations */
SkyFuture* sky_future_create(void);
void sky_future_resolve(SkyFuture* fut, Value res);
void sky_future_reject(SkyFuture* fut, const char* err);
Value sky_future_await(SkyFuture* fut);
Value val_future_await(Value v);

Value sky_async_sleep(int argc, Value* argv);
Value sky_async_all(int argc, Value* argv);
Value sky_async_race(int argc, Value* argv);
Value sky_async_spawn(int argc, Value* argv);

/* Type query & printing */
const char* sky_type_name(SkyTypeId t);
Value val_get_type(Value v);
void val_print(Value v);
void val_println(Value v);
char* val_to_string(Value v);
bool val_is_truthy(Value v);

/* Operators */
Value val_add(Value a, Value b);
Value val_sub(Value a, Value b);
Value val_mul(Value a, Value b);
Value val_div(Value a, Value b);
Value val_floordiv(Value a, Value b);
Value val_pow(Value a, Value b);
Value val_mod(Value a, Value b);
Value val_neg(Value a);
Value val_not(Value a);

Value val_eq(Value a, Value b);
Value val_neq(Value a, Value b);
Value val_lt(Value a, Value b);
Value val_lte(Value a, Value b);
Value val_gt(Value a, Value b);
Value val_gte(Value a, Value b);

/* Indexing & Slicing */
Value val_get_index(Value target, Value index);
Value val_set_index(Value target, Value index, Value val);
Value val_slice(Value target, Value start, Value end);
Value val_unpack(Value coll, size_t index);

/* Property & Member access */
Value val_get_prop(Value target, const char* name);
Value val_set_prop(Value target, const char* name, Value val);
Value val_call_method(Value target, const char* name, int argc, Value* argv);
Value val_call_method_kw(Value target, const char* name, int argc, Value* argv, const char** arg_names);
Value val_call(Value callee, int argc, Value* argv);
Value val_call_kw(Value callee, int argc, Value* argv, const char** arg_names);

/* Collection Methods */
Value list_push(ObjList* list, Value item);
Value list_pop(ObjList* list, int index_or_neg1);
Value list_clear(ObjList* list);
Value list_size(ObjList* list);

Value sorted_list_push(ObjSortedList* sl, Value item);
Value sorted_list_pop(ObjSortedList* sl, int index_or_neg1);
Value sorted_list_size(ObjSortedList* sl);

Value dict_set(ObjDict* dict, Value key, Value value);
Value dict_get(ObjDict* dict, Value key);
bool dict_has(ObjDict* dict, Value key);
Value dict_keys(ObjDict* dict);
Value dict_values(ObjDict* dict);

Value set_add(ObjSet* set, Value item);
bool set_has(ObjSet* set, Value item);
Value set_remove(ObjSet* set, Value item);

/* Built-in helpers */
void sky_init_runtime(void);
Value sky_builtin_eval(int argc, Value* argv);
Value sky_builtin_split(int argc, Value* argv);
Value sky_builtin_join(int argc, Value* argv);
Value sky_builtin_upper(int argc, Value* argv);
Value sky_builtin_lower(int argc, Value* argv);
Value sky_builtin_trim(int argc, Value* argv);
Value sky_builtin_trimleft(int argc, Value* argv);
Value sky_builtin_trimright(int argc, Value* argv);
Value sky_builtin_contains(int argc, Value* argv);
Value sky_builtin_startswith(int argc, Value* argv);
Value sky_builtin_endswith(int argc, Value* argv);
Value sky_builtin_replace(int argc, Value* argv);
Value sky_builtin_find(int argc, Value* argv);
Value sky_builtin_count(int argc, Value* argv);
Value sky_builtin_reverse(int argc, Value* argv);
Value sky_builtin_chars(int argc, Value* argv);
Value sky_builtin_bytes(int argc, Value* argv);
void sky_check_takes(const char* fn_name, int actual_argc, int expected_argc, ...);

/* Traceback and Location Tracking */
void sky_set_loc(const char* file, int line, const char* fn);
void sky_push_frame(const char* file, int line, const char* fn);
void sky_pop_frame(void);
void sky_runtime_error(const char* exc_name, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* SKYLANG_RT_H */
