#ifndef SKY_CPP_H
#define SKY_CPP_H

#include "skylang_rt.h"

/* Initialize the C++ interop bridge */
Value sky_cpp_init(void);

/* Dynamic loading and on-the-fly compilation */
Value sky_cpp_load(const char* so_path);
Value sky_cpp_compile(const char* cpp_code);
Value sky_cpp_exec(const char* cpp_code);

/* Method and property dispatch for C++ OBJ_FOREIGN objects */
Value sky_cpp_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_cpp_get_prop(ObjForeign* f, const char* name);
Value sky_cpp_set_prop(ObjForeign* f, const char* name, Value val);

/* Global cpp module variable */
extern Value sky_mod_cpp;

#endif /* SKY_CPP_H */
