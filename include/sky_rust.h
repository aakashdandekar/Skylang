#ifndef SKY_RUST_H
#define SKY_RUST_H

#include "skylang_rt.h"

/* Initialize the Rust interop bridge */
Value sky_rust_init(void);

/* Dynamic crate / library loading, compilation, and execution */
Value sky_rust_load(const char* path_or_crate);
Value sky_rust_compile(const char* rust_code);
Value sky_rust_exec(const char* code);

/* Method and property dispatch for Rust OBJ_FOREIGN objects */
Value sky_rust_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_rust_get_prop(ObjForeign* f, const char* name);
Value sky_rust_set_prop(ObjForeign* f, const char* name, Value val);

/* Global rust module variable */
extern Value sky_mod_rust;

#endif /* SKY_RUST_H */
