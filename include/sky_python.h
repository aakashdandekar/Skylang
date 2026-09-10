#ifndef SKY_PYTHON_H
#define SKY_PYTHON_H

#include "skylang_rt.h"

/* Initialize the Python interop bridge */
Value sky_python_init(void);

/* Dynamic loading and execution */
Value sky_python_load(const char* module_name);
Value sky_python_exec(const char* code);
void sky_python_set_global(const char* name, Value val);

/* Method and property dispatch for Python OBJ_FOREIGN objects */
Value sky_python_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_python_get_prop(ObjForeign* f, const char* name);
Value sky_python_set_prop(ObjForeign* f, const char* name, Value val);

/* Global python module variable */
extern Value sky_mod_python;

#endif /* SKY_PYTHON_H */
