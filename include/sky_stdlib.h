#ifndef SKY_STDLIB_H
#define SKY_STDLIB_H

#include "skylang_rt.h"

/* Module initializers — each returns a Value (ObjDict) containing the module's functions */
Value sky_stdlib_math_init(void);
Value sky_stdlib_io_init(void);
Value sky_stdlib_fmt_init(void);

/* Initialize all stdlib modules */
void sky_stdlib_init_all(void);

/* Global module variables (set by init_all) */
extern Value sky_mod_math;
extern Value sky_mod_io;
extern Value sky_mod_fmt;

#endif /* SKY_STDLIB_H */
