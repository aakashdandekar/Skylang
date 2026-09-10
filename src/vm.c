
#include "../include/sky_vm.h"
#include "../include/sky_stdlib.h"
#include "../include/sky_python.h"
#include "../include/sky_js.h"
#include "../include/sky_cpp.h"
#include "../include/sky_java.h"
#include "../include/sky_go.h"
#include "../include/sky_rust.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void profiler_init(Profiler* p) {
    memset(p, 0, sizeof(Profiler));
    p->entries = NULL;
    p->count = 0;
    p->capacity = 0;
    p->enabled = false;
}

static ProfileEntry* profiler_get_entry(Profiler* p, const char* name) {
    for (size_t i = 0; i < p->count; i++) {
        if (strcmp(p->entries[i].fn_name, name) == 0)
            return &p->entries[i];
    }

    if (p->count >= p->capacity) {
        p->capacity = p->capacity < 8 ? 8 : p->capacity * 2;
        p->entries = (ProfileEntry*)realloc(p->entries, sizeof(ProfileEntry) * p->capacity);
    }
    ProfileEntry* e = &p->entries[p->count++];
    e->fn_name = strdup(name);
    e->call_count = 0;
    e->total_time_ms = 0;
    e->instruction_count = 0;
    return e;
}

static const char* opcode_name(uint8_t op) {
    switch (op) {
        case OP_CONST: return "OP_CONST";
        case OP_NIL: return "OP_NIL";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_POP: return "OP_POP";
        case OP_DUP: return "OP_DUP";
        case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_FLOORDIV: return "OP_FLOORDIV";
        case OP_MOD: return "OP_MOD";
        case OP_POW: return "OP_POW";
        case OP_NEG: return "OP_NEG";
        case OP_EQ: return "OP_EQ";
        case OP_NEQ: return "OP_NEQ";
        case OP_LT: return "OP_LT";
        case OP_LTE: return "OP_LTE";
        case OP_GT: return "OP_GT";
        case OP_GTE: return "OP_GTE";
        case OP_NOT: return "OP_NOT";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "OP_JUMP_IF_TRUE";
        case OP_LOOP: return "OP_LOOP";
        case OP_CALL: return "OP_CALL";
        case OP_RETURN: return "OP_RETURN";
        case OP_BUILD_LIST: return "OP_BUILD_LIST";
        case OP_BUILD_ARRAY: return "OP_BUILD_ARRAY";
        case OP_BUILD_TUPLE: return "OP_BUILD_TUPLE";
        case OP_BUILD_DICT: return "OP_BUILD_DICT";
        case OP_BUILD_SET: return "OP_BUILD_SET";
        case OP_GET_INDEX: return "OP_GET_INDEX";
        case OP_SET_INDEX: return "OP_SET_INDEX";
        case OP_GET_PROP: return "OP_GET_PROP";
        case OP_SET_PROP: return "OP_SET_PROP";
        case OP_CALL_METHOD: return "OP_CALL_METHOD";
        case OP_SLICE: return "OP_SLICE";
        case OP_CLASS: return "OP_CLASS";
        case OP_GET_THIS: return "OP_GET_THIS";
        case OP_SET_THIS: return "OP_SET_THIS";
        case OP_PRINT: return "OP_PRINT";
        case OP_UNPACK: return "OP_UNPACK";
        case OP_AWAIT: return "OP_AWAIT";
        case OP_SPAWN: return "OP_SPAWN";
        default: return "OP_UNKNOWN";
    }
}

void profiler_report(Profiler* p) {
    fprintf(stderr, "\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  Skylang Performance Profile\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  Total Time:     %.2fms\n", p->total_time_ms);
    fprintf(stderr, "  Instructions:   %zu\n", p->total_instructions);
    fprintf(stderr, "\n");

    if (p->count > 0) {
        fprintf(stderr, "  ─── Per-Function Breakdown ──────────────────────────────\n");
        fprintf(stderr, "  %-20s %6s %10s %10s %6s\n", "Function", "Calls", "Time(ms)", "Instrs", "%Time");
        fprintf(stderr, "  %-20s %6s %10s %10s %6s\n", "────────────────────", "─────", "────────", "──────", "─────");
        for (size_t i = 0; i < p->count; i++) {
            ProfileEntry* e = &p->entries[i];
            double pct = p->total_time_ms > 0 ? (e->total_time_ms / p->total_time_ms * 100.0) : 0;
            fprintf(stderr, "  %-20s %6d %10.2f %10zu %5.1f%%\n",
                    e->fn_name, e->call_count, e->total_time_ms, e->instruction_count, pct);
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "  ─── Instruction Mix ─────────────────────────────────────\n");

    typedef struct { uint8_t op; size_t count; } OpStat;
    OpStat stats[256];
    int stat_count = 0;
    for (int i = 0; i < 256; i++) {
        if (p->op_counts[i] > 0) {
            stats[stat_count].op = (uint8_t)i;
            stats[stat_count].count = p->op_counts[i];
            stat_count++;
        }
    }

    for (int i = 1; i < stat_count; i++) {
        OpStat tmp = stats[i];
        int j = i - 1;
        while (j >= 0 && stats[j].count < tmp.count) {
            stats[j + 1] = stats[j];
            j--;
        }
        stats[j + 1] = tmp;
    }
    int show = stat_count < 10 ? stat_count : 10;
    for (int i = 0; i < show; i++) {
        double pct = p->total_instructions > 0 ? ((double)stats[i].count / p->total_instructions * 100.0) : 0;
        fprintf(stderr, "  %-20s %10zu  %5.1f%%\n", opcode_name(stats[i].op), stats[i].count, pct);
    }
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n\n");
}

void vm_init(VM* vm) {
    memset(vm, 0, sizeof(VM));
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->globals_count = 0;
    vm->last_result = val_nil();
    vm->had_error = false;
    profiler_init(&vm->profiler);
}

void vm_free(VM* vm) {
    for (int i = 0; i < vm->globals_count; i++) {
        free(vm->global_names[i]);
    }
    if (vm->profiler.entries) {
        for (size_t i = 0; i < vm->profiler.count; i++)
            free(vm->profiler.entries[i].fn_name);
        free(vm->profiler.entries);
    }
}

static void vm_push(VM* vm, Value v) {
    if (vm->stack_top - vm->stack >= VM_STACK_MAX) {
        fprintf(stderr, "VM Error: Stack overflow\n");
        vm->had_error = true;
        return;
    }
    *vm->stack_top++ = v;
}

static Value vm_pop(VM* vm) {
    if (vm->stack_top <= vm->stack) {
        fprintf(stderr, "VM Error: Stack underflow\n");
        vm->had_error = true;
        return val_nil();
    }
    return *(--vm->stack_top);
}

static Value vm_peek(VM* vm, int distance) {
    return vm->stack_top[-1 - distance];
}

static int vm_find_global(VM* vm, const char* name) {
    for (int i = 0; i < vm->globals_count; i++) {
        if (strcmp(vm->global_names[i], name) == 0) return i;
    }
    return -1;
}

static void vm_set_global(VM* vm, const char* name, Value value) {
    int idx = vm_find_global(vm, name);
    if (idx >= 0) {
        vm->global_values[idx] = value;
    } else if (vm->globals_count < VM_GLOBALS_MAX) {
        vm->global_names[vm->globals_count] = strdup(name);
        vm->global_values[vm->globals_count] = value;
        vm->globals_count++;
    }
}

static Value vm_get_global(VM* vm, const char* name) {
    int idx = vm_find_global(vm, name);
    if (idx >= 0) return vm->global_values[idx];
    return val_nil();
}

static Value vm_builtin_print(int argc, Value* argv) {
    for (int i = 0; i < argc; i++) {
        val_print(argv[i]);
        if (i + 1 < argc) putchar(' ');
    }
    putchar('\n');
    return val_nil();
}

static Value vm_builtin_input(int argc, Value* argv) {
    if (argc >= 1) {
        val_print(argv[0]);
        fflush(stdout);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return val_nil();
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return val_string(buf);
}

static Value vm_builtin_range(int argc, Value* argv) {
    int64_t start = 0, end = 0, step = 1;
    if (argc == 1) { end = argv[0].as.i; }
    else if (argc >= 2) { start = argv[0].as.i; end = argv[1].as.i; }
    if (argc >= 3) { step = argv[2].as.i; }
    Value res = val_list();
    ObjList* l = as_list(res);
    if (step > 0) {
        for (int64_t i = start; i < end; i += step) list_push(l, val_int(i));
    } else if (step < 0) {
        for (int64_t i = start; i > end; i += step) list_push(l, val_int(i));
    }
    return res;
}

static Value vm_builtin_len(int argc, Value* argv) {
    if (argc >= 1) return val_get_prop(argv[0], "size");
    return val_int(0);
}

static Value vm_builtin_type(int argc, Value* argv) {
    if (argc >= 1) return val_get_type(argv[0]);
    return val_type(TYPE_NIL);
}

static Value vm_builtin_takes(int argc, Value* argv) {
    (void)argc; (void)argv;
    return val_nil();
}

static Value vm_builtin_gc(int argc, Value* argv) {
    (void)argc; (void)argv;
    GC_gcollect();
    return val_nil();
}

static Value vm_builtin_free_fn(int argc, Value* argv) {
    if (argc >= 1 && argv[0].type == VAL_OBJ) GC_FREE(argv[0].as.obj);
    return val_nil();
}

static Value vm_builtin_panic(int argc, Value* argv) {
    fprintf(stderr, "panic: ");
    for (int i = 0; i < argc; i++) { val_print(argv[i]); if (i + 1 < argc) fputc(' ', stderr); }
    fputc('\n', stderr);
    exit(1);
}

static Value vm_builtin_error(int argc, Value* argv) {
    if (argc >= 1) {
        char* s = val_to_string(argv[0]);
        Value err = val_error(s);
        free(s);
        return err;
    }
    return val_error("error");
}

static void vm_register_builtins(VM* vm) {
    vm_set_global(vm, "print",   val_function("print",   vm_builtin_print,    -1));
    vm_set_global(vm, "println", val_function("println", vm_builtin_print,    -1));
    vm_set_global(vm, "input",   val_function("input",   vm_builtin_input,    -1));
    vm_set_global(vm, "eval",    val_function("eval",    sky_builtin_eval,     1));
    vm_set_global(vm, "range",   val_function("range",   vm_builtin_range,    -1));
    vm_set_global(vm, "len",     val_function("len",     vm_builtin_len,       1));
    vm_set_global(vm, "type",    val_function("type",    vm_builtin_type,      1));
    vm_set_global(vm, "takes",   val_function("takes",   vm_builtin_takes,    -1));
    vm_set_global(vm, "gc",      val_function("gc",      vm_builtin_gc,        0));
    vm_set_global(vm, "free",    val_function("free",    vm_builtin_free_fn,   1));
    vm_set_global(vm, "panic",   val_function("panic",   vm_builtin_panic,    -1));
    vm_set_global(vm, "error",   val_function("error",   vm_builtin_error,     1));
    vm_set_global(vm, "split",      val_function("split",      sky_builtin_split,      -1));
    vm_set_global(vm, "join",       val_function("join",       sky_builtin_join,       -1));
    vm_set_global(vm, "upper",      val_function("upper",      sky_builtin_upper,       1));
    vm_set_global(vm, "lower",      val_function("lower",      sky_builtin_lower,       1));
    vm_set_global(vm, "trim",       val_function("trim",       sky_builtin_trim,        1));
    vm_set_global(vm, "trimleft",   val_function("trimleft",   sky_builtin_trimleft,    1));
    vm_set_global(vm, "trimright",  val_function("trimright",  sky_builtin_trimright,   1));
    vm_set_global(vm, "contains",   val_function("contains",   sky_builtin_contains,    2));
    vm_set_global(vm, "startswith", val_function("startswith", sky_builtin_startswith,  2));
    vm_set_global(vm, "endswith",   val_function("endswith",   sky_builtin_endswith,    2));
    vm_set_global(vm, "replace",    val_function("replace",    sky_builtin_replace,     3));
    vm_set_global(vm, "find",       val_function("find",       sky_builtin_find,        2));
    vm_set_global(vm, "count",      val_function("count",      sky_builtin_count,       2));
    vm_set_global(vm, "reverse",    val_function("reverse",    sky_builtin_reverse,     1));
    vm_set_global(vm, "chars",      val_function("chars",      sky_builtin_chars,       1));
    vm_set_global(vm, "bytes",      val_function("bytes",      sky_builtin_bytes,       1));

    sky_stdlib_init_all();
    vm_set_global(vm, "math",  sky_mod_math);
    vm_set_global(vm, "io",    sky_mod_io);
    vm_set_global(vm, "fmt",   sky_mod_fmt);
    vm_set_global(vm, "async", sky_mod_async);

    vm_set_global(vm, "python", sky_python_init());
    vm_set_global(vm, "js",     sky_js_init());
    vm_set_global(vm, "cpp",    sky_cpp_init());
    vm_set_global(vm, "java",   sky_java_init());
    vm_set_global(vm, "golang", sky_go_init());
    vm_set_global(vm, "rust",   sky_rust_init());
}

#define READ_BYTE()   (*frame->ip++)
#define READ_16()     (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONST()  (frame->function->chunk.constants[READ_16()])
#define PUSH(v)       vm_push(vm, v)
#define POP()         vm_pop(vm)
#define PEEK(d)       vm_peek(vm, d)

VMResult vm_run(VM* vm, CompiledFn* main_fn, bool profile) {
    sky_init_runtime();
    vm_register_builtins(vm);
    vm->profiler.enabled = profile;

    CallFrame* frame = &vm->frames[vm->frame_count++];
    frame->function = main_fn;
    frame->ip = main_fn->chunk.code;
    frame->slots = vm->stack;

    struct timespec prof_start, prof_end;
    if (profile) clock_gettime(CLOCK_MONOTONIC, &prof_start);

    for (;;) {
        if (vm->had_error) return VM_RUNTIME_ERROR;

        uint8_t instruction = READ_BYTE();

        if (profile) {
            vm->profiler.op_counts[instruction]++;
            vm->profiler.total_instructions++;
        }

        switch (instruction) {
            case OP_CONST: {
                Value c = READ_CONST();
                PUSH(c);
                break;
            }
            case OP_NIL: PUSH(val_nil()); break;
            case OP_TRUE: PUSH(val_bool(true)); break;
            case OP_FALSE: PUSH(val_bool(false)); break;
            case OP_POP: POP(); break;
            case OP_DUP: PUSH(PEEK(0)); break;

            case OP_GET_LOCAL: {
                uint16_t slot = READ_16();
                PUSH(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint16_t slot = READ_16();
                frame->slots[slot] = PEEK(0);
                break;
            }

            case OP_GET_GLOBAL: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                PUSH(vm_get_global(vm, name));
                break;
            }
            case OP_SET_GLOBAL: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                vm_set_global(vm, name, PEEK(0));
                break;
            }

            case OP_ADD: { Value b = POP(); Value a = POP(); PUSH(val_add(a, b)); break; }
            case OP_SUB: { Value b = POP(); Value a = POP(); PUSH(val_sub(a, b)); break; }
            case OP_MUL: { Value b = POP(); Value a = POP(); PUSH(val_mul(a, b)); break; }
            case OP_DIV: { Value b = POP(); Value a = POP(); PUSH(val_div(a, b)); break; }
            case OP_FLOORDIV: { Value b = POP(); Value a = POP(); PUSH(val_floordiv(a, b)); break; }
            case OP_MOD: { Value b = POP(); Value a = POP(); PUSH(val_mod(a, b)); break; }
            case OP_POW: { Value b = POP(); Value a = POP(); PUSH(val_pow(a, b)); break; }
            case OP_NEG: { Value a = POP(); PUSH(val_neg(a)); break; }

            case OP_EQ:  { Value b = POP(); Value a = POP(); PUSH(val_eq(a, b)); break; }
            case OP_NEQ: { Value b = POP(); Value a = POP(); PUSH(val_neq(a, b)); break; }
            case OP_LT:  { Value b = POP(); Value a = POP(); PUSH(val_lt(a, b)); break; }
            case OP_LTE: { Value b = POP(); Value a = POP(); PUSH(val_lte(a, b)); break; }
            case OP_GT:  { Value b = POP(); Value a = POP(); PUSH(val_gt(a, b)); break; }
            case OP_GTE: { Value b = POP(); Value a = POP(); PUSH(val_gte(a, b)); break; }

            case OP_NOT: { Value a = POP(); PUSH(val_not(a)); break; }

            case OP_JUMP: {
                uint16_t offset = READ_16();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_16();
                if (!val_is_truthy(PEEK(0))) frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_16();
                if (val_is_truthy(PEEK(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_16();
                frame->ip -= offset;
                break;
            }

            case OP_CALL: {
                uint8_t argc = READ_BYTE();
                Value callee = PEEK(argc);

                if (callee.type == VAL_OBJ && callee.as.obj->type == OBJ_FUNCTION) {

                    ObjFunction* fn = (ObjFunction*)callee.as.obj;
                    Value* args = vm->stack_top - argc;
                    Value result = fn->fn(argc, args);
                    vm->stack_top -= (argc + 1);
                    PUSH(result);

                    if (profile) {
                        ProfileEntry* pe = profiler_get_entry(&vm->profiler, fn->name);
                        pe->call_count++;
                    }
                } else {

                    fprintf(stderr, "VM Error [line %d]: Attempt to call a non-function value\n",
                            frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 2]);
                    return VM_RUNTIME_ERROR;
                }
                break;
            }

            case OP_RETURN: {
                Value result = POP();
                vm->frame_count--;
                if (vm->frame_count <= 0) {
                    vm->last_result = result;
                    if (profile) {
                        clock_gettime(CLOCK_MONOTONIC, &prof_end);
                        vm->profiler.total_time_ms =
                            (prof_end.tv_sec - prof_start.tv_sec) * 1000.0 +
                            (prof_end.tv_nsec - prof_start.tv_nsec) / 1e6;
                        profiler_report(&vm->profiler);
                    }
                    return VM_OK;
                }
                vm->stack_top = frame->slots;
                PUSH(result);
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_BUILD_LIST: {
                uint16_t count = READ_16();
                Value list = val_list();
                ObjList* l = as_list(list);
                Value* start = vm->stack_top - count;
                for (uint16_t i = 0; i < count; i++)
                    list_push(l, start[i]);
                vm->stack_top -= count;
                PUSH(list);
                break;
            }

            case OP_BUILD_ARRAY: {
                uint16_t count = READ_16();

                SkyTypeId elem_type = TYPE_INT;
                if (count > 0) {
                    Value first = vm->stack_top[-count];
                    if (first.type == VAL_DOUBLE) elem_type = TYPE_DOUBLE;
                    else if (first.type == VAL_BOOL) elem_type = TYPE_BOOL;
                    else if (first.type == VAL_OBJ && first.as.obj->type == OBJ_STRING) elem_type = TYPE_STRING;
                }
                Value arr = val_array(count, elem_type);
                ObjArray* a = as_array(arr);
                Value* start = vm->stack_top - count;
                for (uint16_t i = 0; i < count; i++)
                    a->items[i] = start[i];
                vm->stack_top -= count;
                PUSH(arr);
                break;
            }

            case OP_BUILD_TUPLE: {
                uint16_t count = READ_16();
                Value* items = (Value*)GC_MALLOC(sizeof(Value) * (count > 0 ? count : 1));
                Value* start = vm->stack_top - count;
                for (uint16_t i = 0; i < count; i++)
                    items[i] = start[i];
                vm->stack_top -= count;
                PUSH(val_tuple(count, items));
                break;
            }

            case OP_BUILD_DICT: {
                uint16_t count = READ_16();
                Value dict = val_dict();
                ObjDict* d = as_dict(dict);
                Value* start = vm->stack_top - count * 2;
                for (uint16_t i = 0; i < count; i++)
                    dict_set(d, start[i * 2], start[i * 2 + 1]);
                vm->stack_top -= count * 2;
                PUSH(dict);
                break;
            }

            case OP_BUILD_SET: {
                uint16_t count = READ_16();
                Value set = val_set();
                ObjSet* s = as_set(set);
                Value* start = vm->stack_top - count;
                for (uint16_t i = 0; i < count; i++)
                    set_add(s, start[i]);
                vm->stack_top -= count;
                PUSH(set);
                break;
            }

            case OP_GET_INDEX: {
                Value idx = POP();
                Value target = POP();
                PUSH(val_get_index(target, idx));
                break;
            }
            case OP_SET_INDEX: {
                Value val = POP();
                Value idx = POP();
                Value target = POP();
                val_set_index(target, idx, val);
                PUSH(val);
                break;
            }
            case OP_GET_PROP: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                Value target = POP();
                PUSH(val_get_prop(target, name));
                break;
            }
            case OP_SET_PROP: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                Value target = POP();
                Value val = POP();
                val_set_prop(target, name, val);
                PUSH(val);
                break;
            }
            case OP_CALL_METHOD: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                uint8_t argc = READ_BYTE();
                Value* args = vm->stack_top - argc;
                Value target = args[-1];
                Value result = val_call_method(target, name, argc, args);
                vm->stack_top -= (argc + 1);
                PUSH(result);
                break;
            }
            case OP_SLICE: {
                Value end = POP();
                Value start = POP();
                Value target = POP();
                PUSH(val_slice(target, start, end));
                break;
            }

            case OP_CLASS: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                PUSH(val_class(name));
                break;
            }
            case OP_GET_THIS: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;

                Value this_val = frame->slots[0];
                PUSH(val_get_prop(this_val, name));
                break;
            }
            case OP_SET_THIS: {
                Value name_val = READ_CONST();
                const char* name = ((ObjString*)name_val.as.obj)->chars;
                Value this_val = frame->slots[0];
                Value val = PEEK(0);
                val_set_prop(this_val, name, val);
                break;
            }

            case OP_PRINT: {
                uint8_t count = READ_BYTE();
                Value* args = vm->stack_top - count;
                for (uint8_t i = 0; i < count; i++) {
                    val_print(args[i]);
                    if (i + 1 < count) putchar(' ');
                }
                putchar('\n');
                vm->stack_top -= count;
                PUSH(val_nil());
                break;
            }

            case OP_UNPACK: {
                uint16_t index = READ_16();
                Value coll = PEEK(0);
                PUSH(val_unpack(coll, index));
                break;
            }

            case OP_AWAIT: {
                Value fut_val = POP();
                PUSH(val_future_await(fut_val));
                break;
            }

            case OP_SPAWN: {
                uint8_t argc = READ_BYTE();
                Value* args = vm->stack_top - argc;
                Value callee = args[-1];
                Value* spawn_args = (Value*)GC_MALLOC(sizeof(Value) * (argc + 1));
                spawn_args[0] = callee;
                for (uint8_t i = 0; i < argc; i++) {
                    spawn_args[i + 1] = args[i];
                }
                vm->stack_top -= (argc + 1);
                Value fut = sky_async_spawn((int)argc + 1, spawn_args);
                PUSH(fut);
                break;
            }

            default: {
                fprintf(stderr, "VM Error: Unknown opcode %d at ip offset %ld\n",
                        instruction, (long)(frame->ip - frame->function->chunk.code - 1));
                return VM_RUNTIME_ERROR;
            }
        }
    }
}

#undef READ_BYTE
#undef READ_16
#undef READ_CONST
#undef PUSH
#undef POP
#undef PEEK

Value vm_eval(const char* code) {
    if (!code || !*code) return val_nil();

    Parser parser;
    parser_init(&parser, code, "<eval>");
    AstNode* ast = parse_program(&parser);
    if (!ast || parser.had_error) {
        return val_nil();
    }

    CompiledFn* fn = compile_ast_eval(ast);
    if (!fn) {
        return val_nil();
    }

    VM vm;
    vm_init(&vm);
    VMResult res = vm_run(&vm, fn, false);
    Value result = (res == VM_OK) ? vm.last_result : val_nil();
    vm_free(&vm);
    return result;
}

Value sky_builtin_eval(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) {
        return val_nil();
    }
    const char* code = ((ObjString*)argv[0].as.obj)->chars;
    return vm_eval(code);
}

