# Changelog

All notable changes to the **Skylang Snippets & IntelliSense** extension will be documented in this file.

## [1.1.0] - 2026-09-08

### Enhanced IntelliSense & Expanded Snippets Library
- **Type-Aware Member Autocompletion**:
  - Container-specific member filtering for `list`, `dict`, `set`, `sortedList`, `string`, `array`, and `tuple` (e.g. `scores.` suggests `.has()`, `tasks.` suggests `.push()`, `.pop()`, `.clear()`, `name.` suggests `.value()`, `.size`, `.T`).
  - Class instance member completion: automatically resolves variable instances to their class definition and suggests all declared methods and fields.
- **Context-Aware Import & FFI Completion**:
  - `import ` auto-completes interop runtime modules (`python`, `js`, `npm`, `cpp`, `java`).
  - `cimport "..."` auto-completes standard C library headers (`math.h`, `stdio.h`, `stdlib.h`, `string.h`, `time.h`, `unistd.h`, etc.).
  - `extern f ` auto-completes common C library signatures (`cos(x)`, `sin(x)`, `tan(x)`, `sqrt(x)`, `atan2(y, x)`, `pow(base, exp)`, `strlen(str)`, `strcmp(s1, s2)`, etc.).
  - `python.load("...")`, `java.load("...")`, `npm.load("...")` auto-completes popular ecosystem packages (NumPy, SciPy, sys, math, java.lang.Math, java.util.ArrayList, lodash, axios, etc.).
- **Smart Parameter & Named Argument Autocompletion**:
  - Interactive named parameter completion (`param_name=`) inside function, method, and constructor calls.
  - Active parameter tracking in signature help for user functions, class constructors, stdlib methods, and extern C declarations.
- **70+ Comprehensive Snippets**:
  - Scalar and inferred variable declarations (`decl_i`, `decl_d`, `decl_b`, `decl_c`, `decl_s`, `decl_def`, `walrus`, `:=`, `multi_decl`, `swap`, `type_check`, `type_fn`).
  - Collections (`arr_decl`, `arr_lit`, `arr_slice`, `list_decl`, `list_lit`, `list_push`, `list_pop`, `list_pop_idx`, `list_clear`, `list_slice`, `tuple_decl`, `tuple_destruct`, `dict_decl`, `dict_lit`, `dict_get`, `dict_set`, `dict_has`, `dict_loop`, `set_decl`, `set_add`, `set_has`, `sl_decl`, `sl_push`, `size`, `len`).
  - Control Flow (`if`, `ifelse`, `ifelif`, `if_not`, `if_err`, `forin`, `forr`, `forr_step`, `forcond`, `forinf`, `for_str`, `break`, `continue`).
  - Functions & Error Handling (`fn`, `fn_void`, `fn_var`, `fn_named`, `fn_safe`, `fn_multireturn`, `err_check`, `err_blank`, `err_only`, `error`, `panic`, `gc`, `free`, `print`, `println`).
  - Object-Oriented Programming (`class`, `class_simple`, `init`, `method`, `method_void`, `getter`, `setter`, `this_field`, `this_call`, `instantiate`, `new_inst`).
  - Standard Library Modules:
    - Math (`math_call`, `math_sqrt`, `math_min_max`, `math_clamp`, `math_hypot`, `math_pow`, `math_constants`).
    - IO (`io_read`, `io_write`, `io_append`, `io_lines`, `io_input`, `io_exists`, `io_remove`).
    - FMT (`fmt_format`, `fmt_hex`, `fmt_bin`, `fmt_oct`, `fmt_pad`, `fmt_padleft`, `fmt_repeat`).
    - STR (`str_split`, `str_join`, `str_upper`, `str_lower`, `str_trim`, `str_replace`, `str_contains`, `str_startswith`, `str_endswith`, `str_find`, `str_count`, `str_reverse`, `str_chars`, `str_bytes`).
  - Multi-Language Interop (`import_python`, `py_load`, `py_eval`, `py_exec`, `py_numpy`, `import_js`, `js_load`, `js_eval`, `import_npm`, `npm_load`, `import_cpp`, `cpp_eval`, `cpp_load`, `import_java`, `java_load`, `java_eval`).
  - C FFI (`cimport`, `extern_fn`).
  - Complete Practical Boilerplates (`boiler_main`, `boiler_oop`, `boiler_error`, `boiler_interop`, `boiler_cli`).
- **Rich Hover Documentation**:
  - Hover on C headers, extern C functions, inferred types, class constructors, method parameters, and stdlib modules.
- **Enhanced Diagnostics**:
  - Flags invalid members accessed on standard library modules.
  - Bracketless function warnings and unclosed brackets/quotes checks.

## [1.0.0] - 2026-09-08

### Initial Release
- **Syntax Highlighting**: Comprehensive TextMate grammar covering keywords, scalar types (`I`, `D`, `B`, `C`, `S`), collection types (`L`, `T`, `SL`, `DICT`, `SET`), operators, string escapes, numbers, comments, and stdlib modules.
- **Snippets**: Productivity snippets covering variables, collections, functions, classes, error handling, standard library, and multi-language interop (Python, JS, C++, Java, FFI).
- **IntelliSense & Autocomplete**:
  - Contextual member completions on `.` for `math.*`, `io.*`, `fmt.*`, `str.*`, `python.*`, `js.*`, `npm.*`, `cpp.*`, `java.*`
  - `this.` class field and method autocompletion
  - Object & collection methods (`push`, `pop`, `clear`, `add`, `has`, `value`, `.size`, `.T`)
  - Workspace & document symbol completions for functions, classes, methods, and variables
  - Named argument completion (`param=`) inside function calls
- **Hover Documentation**: Detailed markdown hover documentation with signatures, parameter tables, return types, and runnable code examples.
- **Signature Help**: Interactive parameter hint popup for functions, constructors, and stdlib methods.
- **Outline & Go to Definition**: Document symbol hierarchy, workspace symbol search (`Ctrl+T`), and Go to Definition (`F12`).
- **Code Formatter**: Automatic document formatting for indentation and operator spacing.
- **Diagnostics & Linter**: Real-time syntax validation, bracket matching, string checking, and bracketless function warnings.
- **CodeLens & Runner**: Integrated commands and CodeLens buttons to run via AOT (`sky run`), execute on Bytecode VM (`sky vm`), profile VM opcodes (`sky profile`), build native binary (`sky build`), and launch the interactive REPL (`sky repl`).
