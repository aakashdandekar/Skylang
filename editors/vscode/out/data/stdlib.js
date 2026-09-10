"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.COMMON_RUST_CRATES = exports.COMMON_GO_PACKAGES = exports.COMMON_EXTERN_C_FUNCTIONS = exports.COMMON_C_HEADERS = exports.COMMON_NPM_PACKAGES = exports.COMMON_JAVA_CLASSES = exports.COMMON_PYTHON_PACKAGES = exports.STDLIB_MODULES = void 0;
exports.STDLIB_MODULES = {
    'math': {
        name: 'math (Standard Math Module)',
        description: 'Built-in mathematical functions and numerical constants available globally in Skylang without import.',
        constants: {
            'pi': { name: 'math.pi', signature: 'math.pi', description: 'The mathematical constant π (3.14159265358979323846...)', returns: 'double', example: 'println("Circumference:", 2 * math.pi * r)' },
            'e': { name: 'math.e', signature: 'math.e', description: 'Euler\'s number e (2.71828182845904523536...)', returns: 'double', example: 'println("e:", math.e)' },
            'inf': { name: 'math.inf', signature: 'math.inf', description: 'Positive floating-point infinity value', returns: 'double', example: 'best_score := -math.inf' },
            'nan': { name: 'math.nan', signature: 'math.nan', description: 'IEEE 754 Not-a-Number value', returns: 'double', example: 'println("NaN:", math.nan)' }
        },
        functions: {
            'sqrt': { name: 'math.sqrt', signature: 'math.sqrt(x)', description: 'Returns the principal square root of non-negative number x.', params: [{ name: 'x', doc: 'Numeric value (int or double)' }], returns: 'double', example: 'math.sqrt(144) // 12.0' },
            'sin': { name: 'math.sin', signature: 'math.sin(x)', description: 'Returns the trigonometric sine of x (where x is in radians).', params: [{ name: 'x', doc: 'Angle in radians' }], returns: 'double', example: 'math.sin(math.pi / 2) // 1.0' },
            'cos': { name: 'math.cos', signature: 'math.cos(x)', description: 'Returns the trigonometric cosine of x (where x is in radians).', params: [{ name: 'x', doc: 'Angle in radians' }], returns: 'double', example: 'math.cos(0) // 1.0' },
            'tan': { name: 'math.tan', signature: 'math.tan(x)', description: 'Returns the trigonometric tangent of x (where x is in radians).', params: [{ name: 'x', doc: 'Angle in radians' }], returns: 'double', example: 'math.tan(0) // 0.0' },
            'asin': { name: 'math.asin', signature: 'math.asin(x)', description: 'Returns the arc sine of x in radians, in the range [-π/2, +π/2].', params: [{ name: 'x', doc: 'Value in the range [-1.0, 1.0]' }], returns: 'double', example: 'math.asin(1.0) // ~1.5708' },
            'acos': { name: 'math.acos', signature: 'math.acos(x)', description: 'Returns the arc cosine of x in radians, in the range [0, π].', params: [{ name: 'x', doc: 'Value in the range [-1.0, 1.0]' }], returns: 'double', example: 'math.acos(1.0) // 0.0' },
            'atan': { name: 'math.atan', signature: 'math.atan(x)', description: 'Returns the arc tangent of x in radians, in the range [-π/2, +π/2].', params: [{ name: 'x', doc: 'Numeric value' }], returns: 'double', example: 'math.atan(1.0) // ~0.7854' },
            'atan2': { name: 'math.atan2', signature: 'math.atan2(y, x)', description: 'Returns the four-quadrant arc tangent of y and x in radians [-π, +π].', params: [{ name: 'y', doc: 'Y coordinate' }, { name: 'x', doc: 'X coordinate' }], returns: 'double', example: 'math.atan2(1.0, 1.0) // ~0.7854' },
            'abs': { name: 'math.abs', signature: 'math.abs(x)', description: 'Returns the absolute value of integer or double x.', params: [{ name: 'x', doc: 'Numeric value' }], returns: 'int | double', example: 'math.abs(-42) // 42' },
            'ceil': { name: 'math.ceil', signature: 'math.ceil(x)', description: 'Returns the smallest integer value greater than or equal to x.', params: [{ name: 'x', doc: 'Numeric value' }], returns: 'double', example: 'math.ceil(4.2) // 5.0' },
            'floor': { name: 'math.floor', signature: 'math.floor(x)', description: 'Returns the largest integer value less than or equal to x.', params: [{ name: 'x', doc: 'Numeric value' }], returns: 'double', example: 'math.floor(4.8) // 4.0' },
            'round': { name: 'math.round', signature: 'math.round(x)', description: 'Rounds x to the nearest integer value.', params: [{ name: 'x', doc: 'Numeric value' }], returns: 'double', example: 'math.round(4.5) // 5.0' },
            'log': { name: 'math.log', signature: 'math.log(x)', description: 'Returns the natural logarithm (base e) of positive number x.', params: [{ name: 'x', doc: 'Positive numeric value' }], returns: 'double', example: 'math.log(math.e) // 1.0' },
            'log10': { name: 'math.log10', signature: 'math.log10(x)', description: 'Returns the common base-10 logarithm of positive number x.', params: [{ name: 'x', doc: 'Positive numeric value' }], returns: 'double', example: 'math.log10(1000) // 3.0' },
            'log2': { name: 'math.log2', signature: 'math.log2(x)', description: 'Returns the binary base-2 logarithm of positive number x.', params: [{ name: 'x', doc: 'Positive numeric value' }], returns: 'double', example: 'math.log2(256) // 8.0' },
            'exp': { name: 'math.exp', signature: 'math.exp(x)', description: 'Returns Euler\'s number e raised to the power x (e^x).', params: [{ name: 'x', doc: 'Numeric exponent' }], returns: 'double', example: 'math.exp(1) // 2.71828...' },
            'pow': { name: 'math.pow', signature: 'math.pow(base, exp)', description: 'Returns base raised to the power exp (base^exp).', params: [{ name: 'base', doc: 'Base number' }, { name: 'exp', doc: 'Exponent' }], returns: 'double', example: 'math.pow(2, 10) // 1024.0' },
            'min': { name: 'math.min', signature: 'math.min(a, b)', description: 'Returns the smaller of two numeric values.', params: [{ name: 'a', doc: 'First number' }, { name: 'b', doc: 'Second number' }], returns: 'int | double', example: 'math.min(10, 20) // 10' },
            'max': { name: 'math.max', signature: 'math.max(a, b)', description: 'Returns the larger of two numeric values.', params: [{ name: 'a', doc: 'First number' }, { name: 'b', doc: 'Second number' }], returns: 'int | double', example: 'math.max(10, 20) // 20' },
            'hypot': { name: 'math.hypot', signature: 'math.hypot(x, y)', description: 'Returns the Euclidean norm sqrt(x² + y²) without intermediate overflow.', params: [{ name: 'x', doc: 'X coordinate' }, { name: 'y', doc: 'Y coordinate' }], returns: 'double', example: 'math.hypot(3, 4) // 5.0' },
            'clamp': { name: 'math.clamp', signature: 'math.clamp(val, min, max)', description: 'Clamps value to lie between the lower bound min and upper bound max.', params: [{ name: 'val', doc: 'Value to clamp' }, { name: 'min', doc: 'Lower bound' }, { name: 'max', doc: 'Upper bound' }], returns: 'int | double', example: 'math.clamp(15, 0, 10) // 10' }
        }
    },
    'python': {
        name: 'python (Python 3 Interoperability Bridge)',
        description: 'Direct embedded Python 3 C API runtime bridge with automatic bidirectional type marshalling for loading NumPy, SciPy, PyTorch, sys, math, and custom Python scripts.',
        functions: {
            'load': { name: 'python.load', signature: 'python.load(module_name, ...)', description: 'Loads one or more Python standard or third-party modules into callable Skylang objects.', params: [{ name: 'module_name', doc: 'Python module name (e.g. "numpy", "pandas", "math", "os", "sys", "torch")' }], returns: 'foreign_obj', example: 'import python\nnp := python.load("numpy")\narr := np.array([1, 2, 3, 4])\nprintln(arr)' },
            'exec': { name: 'python.exec', signature: 'python.exec({\\n  ...\\n})', description: 'Executes Python source code statements, classes, and function definitions inside the embedded Python session.', params: [{ name: 'code_block', doc: 'Python source code block inside curly braces' }], returns: 'none', example: 'import python\npython.exec({\n    def greet(name):\n        return f\'Hello, {name}!\'\n})' },
            'version': { name: 'python.version', signature: 'python.version()', description: 'Returns the version string of the embedded Python runtime.', params: [], returns: 'string', example: 'import python\nprintln("Python:", python.version())' }
        }
    },
    'js': {
        name: 'js (JavaScript / Node.js Interoperability Bridge)',
        description: 'JavaScript / Node.js runtime and global ecosystem bridge with bidirectional JSON marshalling.',
        functions: {
            'load': { name: 'js.load', signature: 'js.load(global_or_pkg)', description: 'Loads a JavaScript global object (e.g. "Math", "JSON") or installed Node.js/NPM package into a callable Skylang object.', params: [{ name: 'global_or_pkg', doc: 'JS global object or NPM package name (e.g. "Math", "lodash", "axios")' }], returns: 'foreign_obj', example: 'import js\nMath_js := js.load("Math")\nprintln(Math_js.sqrt(625))' },
            'exec': { name: 'js.exec', signature: 'js.exec({\\n  ...\\n})', description: 'Executes JavaScript code statements in the Node.js runtime environment.', params: [{ name: 'code_block', doc: 'JavaScript code block inside curly braces' }], returns: 'none', example: 'import js\njs.exec({\n  console.log("Server Started");\n  let a = 10;\n})' }
        }
    },
    'cpp': {
        name: 'cpp (C++ JIT Compilation & Native Bridge)',
        description: 'On-the-fly C++ compilation pipeline via g++ and dynamic linking (`dlopen`) with support for JIT-compiled extern "C" functions.',
        functions: {
            'compile': { name: 'cpp.compile', signature: 'cpp.compile({\\n  ...\\n})', description: 'JIT-compiles C++ source code with extern "C" bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'code_block', doc: 'C++ source code block inside curly braces' }], returns: 'foreign_obj', example: 'import cpp\ncpp_mod := cpp.compile({\n  extern "C" {\n    double fast_add(double a, double b) {\n      return a + b;\n    }\n  }\n})\nprintln(cpp_mod.fast_add(10.5, 20.5))' },
            'load': { name: 'cpp.load', signature: 'cpp.load(so_path)', description: 'Loads a pre-compiled native shared library (.so) into Skylang.', params: [{ name: 'so_path', doc: 'Path to .so shared object library' }], returns: 'foreign_obj', example: 'import cpp\nlib := cpp.load("./libnative.so")' }
        }
    },
    'java': {
        name: 'java (JVM Reflection & Class Bridge)',
        description: 'Java Virtual Machine (OpenJDK) interop bridge supporting class reflection, static method dispatch, property access, and statement execution.',
        functions: {
            'load': { name: 'java.load', signature: 'java.load(class_name, ...)', description: 'Loads one or more Java classes via JVM reflection into callable Skylang objects.', params: [{ name: 'class_name', doc: 'Fully qualified Java class name (e.g. "java.lang.Math", "java.util.ArrayList")' }], returns: 'foreign_obj', example: 'import java\nMath := java.load("java.lang.Math")\nprintln("Java sqrt:", Math.sqrt(256.0))\nprintln("Java PI:", Math.PI)' },
            'exec': { name: 'java.exec', signature: 'java.exec({\\n  ...\\n})', description: 'Executes Java statements and definitions in the JVM environment.', params: [{ name: 'code_block', doc: 'Java code block inside curly braces' }], returns: 'none', example: 'import java\njava.exec({\n  System.out.println("Hello from JVM");\n})' }
        }
    },
    'golang': {
        name: 'golang (Golang JIT Compilation & Native Bridge)',
        description: 'On-the-fly Go compilation via `go build -buildmode=c-shared` and dynamic linking (`dlopen`) with support for exported Go functions.',
        functions: {
            'compile': { name: 'golang.compile', signature: 'golang.compile({\\n  ...\\n})', description: 'JIT-compiles Go source code with `//export` bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'code_block', doc: 'Go source code block inside curly braces' }], returns: 'foreign_obj', example: 'import golang\ngo_mod := golang.compile({\n  //export Add\n  func Add(a, b float64) float64 {\n    return a + b\n  }\n})\nprintln(go_mod.Add(10.5, 20.5))' },
            'load': { name: 'golang.load', signature: 'golang.load(so_path_or_pkg)', description: 'Loads a pre-compiled Go shared library (.so) or Go standard package into Skylang.', params: [{ name: 'so_path_or_pkg', doc: 'Path to .so library or Go package name' }], returns: 'foreign_obj', example: 'import golang\nlib := golang.load("./libcalc.so")' },
            'exec': { name: 'golang.exec', signature: 'golang.exec({\\n  ...\\n})', description: 'Executes Go source code in the Go runtime environment.', params: [{ name: 'code_block', doc: 'Go code block inside curly braces' }], returns: 'none', example: 'import golang\ngolang.exec({\n  println("Hello from Go")\n})' }
        }
    },
    'rust': {
        name: 'rust (Rust JIT Compilation & Native cdylib Bridge)',
        description: 'On-the-fly Rust compilation via `rustc --crate-type cdylib` and dynamic linking (`dlopen`) with support for `pub extern "C"` functions.',
        functions: {
            'compile': { name: 'rust.compile', signature: 'rust.compile({\\n  ...\\n})', description: 'JIT-compiles Rust source code with `#[no_mangle] pub extern "C"` bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'code_block', doc: 'Rust source code block inside curly braces' }], returns: 'foreign_obj', example: 'import rust\nrs_mod := rust.compile({\n  #[no_mangle]\n  pub extern "C" fn add(a: f64, b: f64) -> f64 {\n    a + b\n  }\n})\nprintln(rs_mod.add(10.5, 20.5))' },
            'load': { name: 'rust.load', signature: 'rust.load(so_path_or_module)', description: 'Loads a pre-compiled Rust cdylib library (.so) or standard module into Skylang.', params: [{ name: 'so_path_or_module', doc: 'Path to .so library or Rust module' }], returns: 'foreign_obj', example: 'import rust\nlib := rust.load("./librustcalc.so")' },
            'exec': { name: 'rust.exec', signature: 'rust.exec({\\n  ...\\n})', description: 'Executes Rust source code.', params: [{ name: 'code_block', doc: 'Rust code block inside curly braces' }], returns: 'none', example: 'import rust\nrust.exec({\n  println!("Hello from Rust");\n})' }
        }
    },
    'async': {
        name: 'async (Asynchronous Programming & Concurrency Module)',
        description: 'First-class asynchronous utilities, non-blocking timers, parallel task aggregators, and background thread dispatching.',
        functions: {
            'sleep': {
                name: 'async.sleep',
                signature: 'async.sleep(seconds)',
                description: 'Creates a non-blocking timer that returns a Future resolving after the specified number of seconds.',
                params: [{ name: 'seconds', doc: 'Duration to sleep in seconds (e.g. 0.5, 1.0, 2)' }],
                returns: 'future',
                example: 'await async.sleep(1.0)'
            },
            'all': {
                name: 'async.all',
                signature: 'async.all(futures_list)',
                description: 'Waits for all Future objects in the list to complete in parallel, returning a Future that resolves to a list of results.',
                params: [{ name: 'futures_list', doc: 'List of Future objects' }],
                returns: 'future',
                example: 'results := await async.all([task1, task2, task3])'
            },
            'race': {
                name: 'async.race',
                signature: 'async.race(futures_list)',
                description: 'Waits for the fastest Future object in the list to complete, returning a Future resolving with its result.',
                params: [{ name: 'futures_list', doc: 'List of Future objects' }],
                returns: 'future',
                example: 'winner := await async.race([task1, task2])'
            },
            'spawn': {
                name: 'async.spawn',
                signature: 'async.spawn(fn, ...args)',
                description: 'Dynamically launches a function with arguments onto a background worker thread and returns a Future immediately.',
                params: [
                    { name: 'fn', doc: 'Callable function or function pointer' },
                    { name: '...args', doc: 'Optional arguments to pass to the function' }
                ],
                returns: 'future',
                example: 'fut := async.spawn(heavy_computation, 1000000)\nres := await fut'
            }
        }
    }
};
exports.COMMON_PYTHON_PACKAGES = [
    'numpy', 'pandas', 'torch', 'scipy', 'matplotlib', 'sklearn', 'requests',
    'sys', 'os', 'math', 'json', 'random', 're', 'datetime', 'collections',
    'itertools', 'functools', 'typing', 'csv', 'pathlib', 'http', 'urllib',
    'sqlite3', 'threading', 'multiprocessing', 'subprocess', 'shutil', 'pickle',
    'glob', 'hashlib', 'logging', 'time', 'socket', 'ctypes', 'io', 'base64',
    'asyncio', 'tensorflow', 'flask', 'fastapi', 'django', 'bs4', 'PIL', 'cv2',
    'polars', 'transformers', 'openai', 'anthropic', 'rich', 'tqdm', 'click',
    'typer', 'yaml', 'pydantic', 'pytest', 'seaborn'
];
exports.COMMON_JAVA_CLASSES = [
    'java.lang.Math', 'java.lang.String', 'java.lang.System', 'java.lang.Integer',
    'java.lang.Double', 'java.lang.Boolean', 'java.util.ArrayList', 'java.util.HashMap',
    'java.util.HashSet', 'java.util.Arrays', 'java.util.Collections', 'java.util.Random',
    'java.util.Scanner', 'java.io.File', 'java.time.LocalDate', 'java.time.LocalDateTime',
    'java.net.URI', 'java.net.http.HttpClient'
];
exports.COMMON_NPM_PACKAGES = [
    'lodash', 'axios', 'express', 'expressjs', 'moment', 'chalk', 'fs', 'path', 'http',
    'crypto', 'dotenv', 'rxjs', 'ws', 'dayjs', 'zod', 'commander', 'glob',
    'fs-extra', 'cheerio', 'debug', 'yargs', 'winston', 'bcrypt', 'cors', 'jsonwebtoken'
];
exports.COMMON_C_HEADERS = [
    'math.h', 'stdio.h', 'stdlib.h', 'string.h', 'time.h', 'unistd.h',
    'ctype.h', 'limits.h', 'float.h', 'stdbool.h', 'stdint.h', 'errno.h',
    'assert.h', 'sys/time.h', 'sys/stat.h', 'fcntl.h', 'pthread.h'
];
exports.COMMON_EXTERN_C_FUNCTIONS = {
    'cos': { signature: 'extern f cos(x)', header: 'math.h', doc: 'C standard cosine function' },
    'sin': { signature: 'extern f sin(x)', header: 'math.h', doc: 'C standard sine function' },
    'tan': { signature: 'extern f tan(x)', header: 'math.h', doc: 'C standard tangent function' },
    'sqrt': { signature: 'extern f sqrt(x)', header: 'math.h', doc: 'C standard square root function' },
    'atan2': { signature: 'extern f atan2(y, x)', header: 'math.h', doc: 'C standard four-quadrant arc tangent' },
    'pow': { signature: 'extern f pow(base, exp)', header: 'math.h', doc: 'C standard power function' },
    'fabs': { signature: 'extern f fabs(x)', header: 'math.h', doc: 'C standard absolute value function' },
    'ceil': { signature: 'extern f ceil(x)', header: 'math.h', doc: 'C standard ceiling function' },
    'floor': { signature: 'extern f floor(x)', header: 'math.h', doc: 'C standard floor function' },
    'exp': { signature: 'extern f exp(x)', header: 'math.h', doc: 'C standard exponential function' },
    'log': { signature: 'extern f log(x)', header: 'math.h', doc: 'C standard natural logarithm function' },
    'strlen': { signature: 'extern f strlen(str)', header: 'string.h', doc: 'C standard string length function' },
    'strcmp': { signature: 'extern f strcmp(s1, s2)', header: 'string.h', doc: 'C standard string comparison function' },
    'time': { signature: 'extern f time(ptr)', header: 'time.h', doc: 'C standard time function' },
    'clock': { signature: 'extern f clock()', header: 'time.h', doc: 'C standard processor clock time' }
};
exports.COMMON_GO_PACKAGES = [
    'fmt', 'math', 'os', 'strings', 'strconv', 'time', 'io', 'net/http',
    'encoding/json', 'sync', 'bytes', 'bufio', 'sort', 'path/filepath', 'crypto/sha256'
];
exports.COMMON_RUST_CRATES = [
    'std::f64::consts', 'std::collections', 'std::fs', 'std::io', 'std::time',
    'serde', 'serde_json', 'tokio', 'rayon', 'regex', 'rand', 'chrono'
];
//# sourceMappingURL=stdlib.js.map