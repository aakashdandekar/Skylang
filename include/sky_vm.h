#ifndef SKY_VM_H
#define SKY_VM_H

#include "skylang_rt.h"
#include "skylang.h"
#include <stdint.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Opcodes
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef enum {
    /* Constants & Stack */
    OP_CONST,           /* Push constant from pool (16-bit index) */
    OP_NIL,             /* Push nil */
    OP_TRUE,            /* Push true */
    OP_FALSE,           /* Push false */
    OP_POP,             /* Pop top of stack */
    OP_DUP,             /* Duplicate top of stack */

    /* Variables */
    OP_GET_GLOBAL,      /* Get global by name index (16-bit) */
    OP_SET_GLOBAL,      /* Set global by name index (16-bit) */
    OP_GET_LOCAL,       /* Get local by stack slot (16-bit) */
    OP_SET_LOCAL,       /* Set local by stack slot (16-bit) */

    /* Arithmetic */
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_FLOORDIV,
    OP_MOD,
    OP_POW,
    OP_NEG,

    /* Comparison */
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,

    /* Logic */
    OP_NOT,

    /* Control Flow */
    OP_JUMP,            /* Unconditional jump (16-bit offset) */
    OP_JUMP_IF_FALSE,   /* Conditional jump (16-bit offset) */
    OP_JUMP_IF_TRUE,    /* Short-circuit OR (16-bit offset) */
    OP_LOOP,            /* Backwards jump (16-bit offset) */

    /* Functions */
    OP_CALL,            /* Call function with N args (8-bit argc) */
    OP_RETURN,          /* Return from function */

    /* Collections */
    OP_BUILD_LIST,      /* Build list from N stack values (16-bit count) */
    OP_BUILD_ARRAY,     /* Build array from N stack values (16-bit count) */
    OP_BUILD_TUPLE,     /* Build tuple from N stack values (16-bit count) */
    OP_BUILD_DICT,      /* Build dict from N key-value pairs (16-bit count) */
    OP_BUILD_SET,       /* Build set from N stack values (16-bit count) */

    /* Property & Index */
    OP_GET_INDEX,       /* collection[index] */
    OP_SET_INDEX,       /* collection[index] = val */
    OP_GET_PROP,        /* obj.prop (16-bit name index) */
    OP_SET_PROP,        /* obj.prop = val (16-bit name index) */
    OP_CALL_METHOD,     /* obj.method(args) (16-bit name index, 8-bit argc) */
    OP_SLICE,           /* collection[start:end] */

    /* OOP */
    OP_CLASS,           /* Define a class (16-bit name index) */
    OP_GET_THIS,        /* Access this.field (16-bit name index) */
    OP_SET_THIS,        /* Set this.field (16-bit name index) */

    /* Special */
    OP_PRINT,           /* Print N values (8-bit count) */
    OP_UNPACK,          /* Tuple unpacking (16-bit index) */
} OpCode;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Bytecode Chunk
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct {
    uint8_t* code;          /* Bytecode array */
    size_t count;
    size_t capacity;
    int* lines;             /* Source line number per byte */
    Value* constants;       /* Constant pool */
    size_t const_count;
    size_t const_capacity;
} Chunk;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Compiled Function
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct {
    char* name;             /* Function name (or "<script>") */
    int arity;              /* Parameter count (-1 for variadic) */
    char** params;          /* Parameter names */
    int param_count;
    Chunk chunk;            /* Bytecode for this function */
    int local_count;        /* Number of local variable slots */
} CompiledFn;

/* ═══════════════════════════════════════════════════════════════════════════
 *  VM Call Frame
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct {
    CompiledFn* function;
    uint8_t* ip;            /* Instruction pointer */
    Value* slots;           /* Frame base pointer into stack */
} CallFrame;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Profiler
 * ═══════════════════════════════════════════════════════════════════════════ */
typedef struct {
    char* fn_name;
    int call_count;
    double total_time_ms;
    size_t instruction_count;
} ProfileEntry;

typedef struct {
    ProfileEntry* entries;
    size_t count;
    size_t capacity;
    size_t op_counts[256];      /* Per-opcode instruction counts */
    double total_time_ms;
    size_t total_instructions;
    bool enabled;
} Profiler;

/* ═══════════════════════════════════════════════════════════════════════════
 *  The Virtual Machine
 * ═══════════════════════════════════════════════════════════════════════════ */
#define VM_STACK_MAX 65536
#define VM_FRAMES_MAX 256
#define VM_GLOBALS_MAX 2048

typedef struct {
    CallFrame frames[VM_FRAMES_MAX];
    int frame_count;

    Value stack[VM_STACK_MAX];
    Value* stack_top;

    /* Global variables: parallel arrays for name → value */
    char* global_names[VM_GLOBALS_MAX];
    Value global_values[VM_GLOBALS_MAX];
    int globals_count;

    Profiler profiler;
    Value last_result;
    bool had_error;
} VM;

typedef enum {
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
} VMResult;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Chunk operations */
void chunk_init(Chunk* chunk);
void chunk_write(Chunk* chunk, uint8_t byte, int line);
size_t chunk_add_constant(Chunk* chunk, Value value);
void chunk_free(Chunk* chunk);

/* Compiler: AST → Bytecode */
CompiledFn* compile_ast(AstNode* program);
CompiledFn* compile_ast_eval(AstNode* program);

/* VM */
void vm_init(VM* vm);
VMResult vm_run(VM* vm, CompiledFn* main_fn, bool profile);
Value vm_eval(const char* code);
Value sky_builtin_eval(int argc, Value* argv);
void vm_free(VM* vm);

/* Profiler */
void profiler_init(Profiler* p);
void profiler_report(Profiler* p);

#endif /* SKY_VM_H */
