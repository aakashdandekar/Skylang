#ifndef SKY_JAVA_H
#define SKY_JAVA_H

#include "skylang_rt.h"

/* Initialize the Java interop bridge */
Value sky_java_init(void);

/* Dynamic Java class loading and execution */
Value sky_java_load(const char* class_name);
Value sky_java_exec(const char* code);

/* Method and property dispatch for Java OBJ_FOREIGN objects */
Value sky_java_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_java_get_prop(ObjForeign* f, const char* name);
Value sky_java_set_prop(ObjForeign* f, const char* name, Value val);

/* Global java module variable */
extern Value sky_mod_java;

#endif /* SKY_JAVA_H */
