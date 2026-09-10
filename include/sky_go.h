#ifndef SKY_GO_H
#define SKY_GO_H

#include "skylang_rt.h"

/* Initialize the Golang interop bridge */
Value sky_go_init(void);

/* Dynamic package / library loading, compilation, and execution */
Value sky_go_load(const char* pkg_or_so);
Value sky_go_compile(const char* go_code);
Value sky_go_exec(const char* code);

/* Method and property dispatch for Go OBJ_FOREIGN objects */
Value sky_go_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_go_get_prop(ObjForeign* f, const char* name);
Value sky_go_set_prop(ObjForeign* f, const char* name, Value val);

/* Global golang module variable */
extern Value sky_mod_golang;

#endif /* SKY_GO_H */
