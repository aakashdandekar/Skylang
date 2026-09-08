#ifndef SKY_JS_H
#define SKY_JS_H

#include "skylang_rt.h"

/* Initialize the JavaScript & NPM interop bridge */
Value sky_js_init(void);

/* Dynamic loading and execution (auto-detects JS globals and NPM packages) */
Value sky_js_load(const char* package_name);
Value sky_js_exec(const char* code);

/* Method and property dispatch for JS OBJ_FOREIGN objects */
Value sky_js_call_method(ObjForeign* f, const char* name, int argc, Value* argv);
Value sky_js_get_prop(ObjForeign* f, const char* name);
Value sky_js_set_prop(ObjForeign* f, const char* name, Value val);

extern Value sky_mod_js;

#endif /* SKY_JS_H */
