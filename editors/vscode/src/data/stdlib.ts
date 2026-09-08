import { BuiltinDoc } from './builtins';

export interface ModuleDoc {
    name: string;
    description: string;
    functions: Record<string, BuiltinDoc>;
    constants?: Record<string, BuiltinDoc>;
}

export const STDLIB_MODULES: Record<string, ModuleDoc> = {
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
    'io': {
        name: 'io (Standard Input / Output Module)',
        description: 'Built-in file reading, file writing, directory inspection, and terminal console input utilities available globally.',
        functions: {
            'readfile': { name: 'io.readfile', signature: 'io.readfile(filepath)', description: 'Reads the entire contents of a file from disk into a string.', params: [{ name: 'filepath', doc: 'Path to the file to read' }], returns: 'string', example: 'content := io.readfile("data.txt")' },
            'writefile': { name: 'io.writefile', signature: 'io.writefile(filepath, content)', description: 'Writes string content to a file, creating it if needed and overwriting existing contents.', params: [{ name: 'filepath', doc: 'Target file path' }, { name: 'content', doc: 'String content to write' }], returns: 'bool', example: 'io.writefile("output.txt", "Hello Skylang!")' },
            'appendfile': { name: 'io.appendfile', signature: 'io.appendfile(filepath, content)', description: 'Appends string content to the end of a file without overwriting existing data.', params: [{ name: 'filepath', doc: 'Target file path' }, { name: 'content', doc: 'String content to append' }], returns: 'bool', example: 'io.appendfile("log.txt", "New entry\\n")' },
            'readlines': { name: 'io.readlines', signature: 'io.readlines(filepath)', description: 'Reads all lines from a file and returns them as a dynamic list of strings.', params: [{ name: 'filepath', doc: 'Path to the file' }], returns: 'list', example: 'lines := io.readlines("config.txt")\nfor line in lines {\n    println(line)\n}' },
            'input': { name: 'io.input', signature: 'io.input([prompt])', description: 'Displays an optional prompt message and reads a single line of input from standard input.', params: [{ name: 'prompt', doc: 'Optional prompt text to display' }], returns: 'string', example: 'name := io.input("Enter your name: ")' },
            'exists': { name: 'io.exists', signature: 'io.exists(filepath)', description: 'Checks whether a file or directory exists at the given path on the filesystem.', params: [{ name: 'filepath', doc: 'Path to test' }], returns: 'bool', example: 'if io.exists("data.txt") {\n    data := io.readfile("data.txt")\n}' },
            'remove': { name: 'io.remove', signature: 'io.remove(filepath)', description: 'Deletes a file from disk. Returns true if successfully removed, otherwise false.', params: [{ name: 'filepath', doc: 'Path to file to delete' }], returns: 'bool', example: 'io.remove("temp.txt")' }
        }
    },
    'fmt': {
        name: 'fmt (Formatting Module)',
        description: 'String templating with `{}` placeholders, numeric base conversion (hex, bin, oct), padding, and string repetition.',
        functions: {
            'format': { name: 'fmt.format', signature: 'fmt.format(template, ...args)', description: 'Formats a string template by replacing each `{}` placeholder with the string representation of subsequent arguments.', params: [{ name: 'template', doc: 'Format string containing {} placeholders' }, { name: '...args', doc: 'Values to interpolate' }], returns: 'string', example: 'msg := fmt.format("User {} scored {}/{}", "Alice", 95, 100)' },
            'hex': { name: 'fmt.hex', signature: 'fmt.hex(integer)', description: 'Converts an integer to its lowercase hexadecimal string representation.', params: [{ name: 'integer', doc: 'Integer number' }], returns: 'string', example: 'fmt.hex(255) // "ff"' },
            'bin': { name: 'fmt.bin', signature: 'fmt.bin(integer)', description: 'Converts an integer to its binary string representation (e.g. "101010").', params: [{ name: 'integer', doc: 'Integer number' }], returns: 'string', example: 'fmt.bin(42) // "101010"' },
            'oct': { name: 'fmt.oct', signature: 'fmt.oct(integer)', description: 'Converts an integer to its octal string representation.', params: [{ name: 'integer', doc: 'Integer number' }], returns: 'string', example: 'fmt.oct(64) // "100"' },
            'pad': { name: 'fmt.pad', signature: 'fmt.pad(str, width)', description: 'Right-pads a string with spaces until it reaches the specified width.', params: [{ name: 'str', doc: 'Input string' }, { name: 'width', doc: 'Target total character width' }], returns: 'string', example: 'fmt.pad("Sky", 6) // "Sky   "' },
            'padleft': { name: 'fmt.padleft', signature: 'fmt.padleft(str, width)', description: 'Left-pads a string with spaces until it reaches the specified width.', params: [{ name: 'str', doc: 'Input string' }, { name: 'width', doc: 'Target total character width' }], returns: 'string', example: 'fmt.padleft("42", 5) // "   42"' },
            'repeat': { name: 'fmt.repeat', signature: 'fmt.repeat(str, count)', description: 'Returns a new string consisting of `str` repeated `count` times.', params: [{ name: 'str', doc: 'String to repeat' }, { name: 'count', doc: 'Number of repetitions' }], returns: 'string', example: 'fmt.repeat("=-", 5) // "=-=-=-=-=-"' }
        }
    },
    'str': {
        name: 'str (String Utilities Module)',
        description: 'Comprehensive string manipulation and transformation functions: split, join, trim, case conversion, search, and byte extraction.',
        functions: {
            'split': { name: 'str.split', signature: 'str.split(str, delimiter)', description: 'Splits string `str` by `delimiter` into a dynamic list of substring strings. If delimiter is empty, splits into individual characters.', params: [{ name: 'str', doc: 'Input string' }, { name: 'delimiter', doc: 'Separator delimiter string' }], returns: 'list', example: 'words := str.split("apple,banana,cherry", ",")' },
            'join': { name: 'str.join', signature: 'str.join(list, delimiter)', description: 'Joins all elements of a list into a single string separated by `delimiter`.', params: [{ name: 'list', doc: 'List of elements to join' }, { name: 'delimiter', doc: 'Separator delimiter string' }], returns: 'string', example: 'joined := str.join(["a", "b", "c"], " - ") // "a - b - c"' },
            'upper': { name: 'str.upper', signature: 'str.upper(str)', description: 'Returns a new string with all ASCII alphabetic characters converted to uppercase.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.upper("skylang") // "SKYLANG"' },
            'lower': { name: 'str.lower', signature: 'str.lower(str)', description: 'Returns a new string with all ASCII alphabetic characters converted to lowercase.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.lower("SKYLANG") // "skylang"' },
            'trim': { name: 'str.trim', signature: 'str.trim(str)', description: 'Returns a new string with all leading and trailing whitespace characters removed.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.trim("   hello world   ") // "hello world"' },
            'trimleft': { name: 'str.trimleft', signature: 'str.trimleft(str)', description: 'Returns a new string with all leading whitespace characters removed.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.trimleft("   code") // "code"' },
            'trimright': { name: 'str.trimright', signature: 'str.trimright(str)', description: 'Returns a new string with all trailing whitespace characters removed.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.trimright("code   ") // "code"' },
            'contains': { name: 'str.contains', signature: 'str.contains(str, substr)', description: 'Returns true if `substr` is found anywhere within `str`, otherwise false.', params: [{ name: 'str', doc: 'Haystack string' }, { name: 'substr', doc: 'Needle substring to search for' }], returns: 'bool', example: 'str.contains("Skylang", "lan") // true' },
            'startswith': { name: 'str.startswith', signature: 'str.startswith(str, prefix)', description: 'Returns true if `str` begins with the specified `prefix` string.', params: [{ name: 'str', doc: 'Input string' }, { name: 'prefix', doc: 'Prefix to check' }], returns: 'bool', example: 'str.startswith("Skylang", "Sky") // true' },
            'endswith': { name: 'str.endswith', signature: 'str.endswith(str, suffix)', description: 'Returns true if `str` ends with the specified `suffix` string.', params: [{ name: 'str', doc: 'Input string' }, { name: 'suffix', doc: 'Suffix to check' }], returns: 'bool', example: 'str.endswith("file.sky", ".sky") // true' },
            'replace': { name: 'str.replace', signature: 'str.replace(str, old_sub, new_sub)', description: 'Returns a new string where all occurrences of `old_sub` are replaced by `new_sub`.', params: [{ name: 'str', doc: 'Input string' }, { name: 'old_sub', doc: 'Substring to replace' }, { name: 'new_sub', doc: 'Replacement substring' }], returns: 'string', example: 'str.replace("banana", "a", "o") // "bonono"' },
            'find': { name: 'str.find', signature: 'str.find(str, substr)', description: 'Returns the 0-based index of the first occurrence of `substr` in `str`, or -1 if not found.', params: [{ name: 'str', doc: 'Input string' }, { name: 'substr', doc: 'Substring to find' }], returns: 'int', example: 'str.find("Skylang", "lang") // 3' },
            'count': { name: 'str.count', signature: 'str.count(str, substr)', description: 'Returns the number of non-overlapping occurrences of `substr` in `str`.', params: [{ name: 'str', doc: 'Input string' }, { name: 'substr', doc: 'Substring to count' }], returns: 'int', example: 'str.count("banana", "a") // 3' },
            'reverse': { name: 'str.reverse', signature: 'str.reverse(str)', description: 'Returns a new string with the characters of `str` in reverse order.', params: [{ name: 'str', doc: 'Input string' }], returns: 'string', example: 'str.reverse("Skylang") // "gnalykS"' },
            'chars': { name: 'str.chars', signature: 'str.chars(str)', description: 'Returns a dynamic list containing each character of `str` as an individual string element.', params: [{ name: 'str', doc: 'Input string' }], returns: 'list', example: 'str.chars("Sky") // ["S", "k", "y"]' },
            'bytes': { name: 'str.bytes', signature: 'str.bytes(str)', description: 'Returns a dynamic list containing the integer ASCII byte values of `str`.', params: [{ name: 'str', doc: 'Input string' }], returns: 'list', example: 'str.bytes("ABC") // [65, 66, 67]' }
        }
    },
    'python': {
        name: 'python (Python 3 Interoperability Bridge)',
        description: 'Direct embedded Python 3 C API runtime bridge with automatic bidirectional type marshalling for loading NumPy, SciPy, PyTorch, sys, math, and custom Python scripts.',
        functions: {
            'load': { name: 'python.load', signature: 'python.load(module_name, ...)', description: 'Loads one or more Python standard or third-party modules into callable Skylang objects.', params: [{ name: 'module_name', doc: 'Python module name (e.g. "numpy", "pandas", "math", "os", "sys", "torch")' }], returns: 'foreign_obj', example: 'import python\nnp := python.load("numpy")\narr := np.array([1, 2, 3, 4])\nprintln(arr)' },
            'import': { name: 'python.import', signature: 'python.import(module_name, ...)', description: 'Alias for `python.load(...)` to import Python packages.', params: [{ name: 'module_name', doc: 'Python module name (e.g. "numpy", "torch")' }], returns: 'foreign_obj', example: 'import python\nnp := python.import("numpy")' },
            'exec': { name: 'python.exec', signature: 'python.exec(code_str)', description: 'Executes Python source code statements, classes, and function definitions inside the embedded Python session.', params: [{ name: 'code_str', doc: 'Python source code block' }], returns: 'none', example: 'import python\npython.exec("\ndef greet(name):\n    return f\'Hello, {name}!\'\n")\nprintln(python.call("greet", "Skylang"))' },
            'call': { name: 'python.call', signature: 'python.call(fn_name, ...args)', description: 'Invokes a globally defined Python function with arguments.', params: [{ name: 'fn_name', doc: 'Name of the Python function' }, { name: '...args', doc: 'Arguments to pass to the function' }], returns: 'any', example: 'import python\nres := python.call("sum", [1, 2, 3, 4, 5])' },
            'get': { name: 'python.get', signature: 'python.get(attr_name)', description: 'Retrieves an attribute or variable from Python globals.', params: [{ name: 'attr_name', doc: 'Name of the attribute or global variable' }], returns: 'any', example: 'import python\nver := python.get("__version__")' },
            'set': { name: 'python.set', signature: 'python.set(attr_name, value)', description: 'Sets a global variable or attribute in the embedded Python interpreter.', params: [{ name: 'attr_name', doc: 'Name of the attribute' }, { name: 'value', doc: 'Value to assign' }], returns: 'none', example: 'import python\npython.set("MAX_RETRIES", 5)' },
            'is_init': { name: 'python.is_init', signature: 'python.is_init()', description: 'Checks whether the embedded Python 3 runtime interpreter is currently initialized.', params: [], returns: 'bool', example: 'import python\nif python.is_init() {\n    println("Python runtime active")\n}' },
            'version': { name: 'python.version', signature: 'python.version()', description: 'Returns the version string of the embedded Python runtime.', params: [], returns: 'string', example: 'import python\nprintln("Python:", python.version())' }
        }
    },
    'js': {
        name: 'js (JavaScript / Node.js Interoperability Bridge)',
        description: 'JavaScript / Node.js runtime and global ecosystem bridge with bidirectional JSON marshalling.',
        functions: {
            'load': { name: 'js.load', signature: 'js.load(global_or_pkg)', description: 'Loads a JavaScript global object (e.g. "Math", "JSON") or installed Node.js/NPM package into a callable Skylang object.', params: [{ name: 'global_or_pkg', doc: 'JS global object or NPM package name (e.g. "Math", "lodash", "axios")' }], returns: 'foreign_obj', example: 'import js\nMath_js := js.load("Math")\nprintln(Math_js.sqrt(625))' },
            'import': { name: 'js.import', signature: 'js.import(package_name)', description: 'Alias for `js.load(...)`.', params: [{ name: 'package_name', doc: 'Package name' }], returns: 'foreign_obj', example: 'import js\n_ := js.import("lodash")' },
            'exec': { name: 'js.exec', signature: 'js.exec(code_str)', description: 'Executes JavaScript code statements in the Node.js runtime environment.', params: [{ name: 'code_str', doc: 'JavaScript code block' }], returns: 'none', example: 'import js\njs.exec("console.log(\'Hello from Node.js\');")' }
        }
    },
    'npm': {
        name: 'npm (NPM Package Interoperability Bridge)',
        description: 'NPM package loader interface for seamless access to the NPM ecosystem (powered by Node.js).',
        functions: {
            'load': { name: 'npm.load', signature: 'npm.load(package_name)', description: 'Loads an installed NPM package into Skylang.', params: [{ name: 'package_name', doc: 'NPM package name (e.g. "lodash", "axios", "chalk", "moment")' }], returns: 'foreign_obj', example: 'import js\n_ := npm.load("lodash")' },
            'import': { name: 'npm.import', signature: 'npm.import(package_name)', description: 'Alias for `npm.load(...)`.', params: [{ name: 'package_name', doc: 'NPM package name' }], returns: 'foreign_obj' },
            'exec': { name: 'npm.exec', signature: 'npm.exec(code_str)', description: 'Executes JS/NPM statements.', params: [{ name: 'code_str', doc: 'Code string' }], returns: 'none' }
        }
    },
    'cpp': {
        name: 'cpp (C++ JIT Compilation & Native Bridge)',
        description: 'On-the-fly C++ compilation pipeline via g++ and dynamic linking (`dlopen`) with support for JIT-compiled extern "C" functions.',
        functions: {
            'compile': { name: 'cpp.compile', signature: 'cpp.compile(cpp_source_code)', description: 'JIT-compiles C++ source code with extern "C" bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'cpp_source_code', doc: 'C++ source code string' }], returns: 'foreign_obj', example: 'import cpp\ncpp_mod := cpp.compile("\nextern \\\"C\\\" {\n    double fast_add(double a, double b) {\n        return a + b;\n    }\n}\n")\nprintln(cpp_mod.fast_add(10.5, 20.5))' },
            'load': { name: 'cpp.load', signature: 'cpp.load(so_path)', description: 'Loads a pre-compiled native shared library (.so) into Skylang.', params: [{ name: 'so_path', doc: 'Path to .so shared object library' }], returns: 'foreign_obj', example: 'import cpp\nlib := cpp.load("./libnative.so")' }
        }
    },
    'java': {
        name: 'java (JVM Reflection & Class Bridge)',
        description: 'Java Virtual Machine (OpenJDK) interop bridge supporting class reflection, static method dispatch, property access, and statement execution.',
        functions: {
            'load': { name: 'java.load', signature: 'java.load(class_name, ...)', description: 'Loads one or more Java classes via JVM reflection into callable Skylang objects.', params: [{ name: 'class_name', doc: 'Fully qualified Java class name (e.g. "java.lang.Math", "java.util.ArrayList")' }], returns: 'foreign_obj', example: 'import java\nMath := java.load("java.lang.Math")\nprintln("Java sqrt:", Math.sqrt(256.0))\nprintln("Java PI:", Math.PI)' },
            'import': { name: 'java.import', signature: 'java.import(class_name, ...)', description: 'Alias for `java.load(...)`.', params: [{ name: 'class_name', doc: 'Java class name' }], returns: 'foreign_obj' },
            'exec': { name: 'java.exec', signature: 'java.exec(code_str)', description: 'Executes Java statements and definitions in the JVM environment.', params: [{ name: 'code_str', doc: 'Java code block' }], returns: 'none' }
        }
    },
    'go': {
        name: 'go (Golang JIT Compilation & Native Bridge)',
        description: 'On-the-fly Go compilation via `go build -buildmode=c-shared` and dynamic linking (`dlopen`) with support for exported Go functions.',
        functions: {
            'compile': { name: 'go.compile', signature: 'go.compile(go_source_code)', description: 'JIT-compiles Go source code with `//export` bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'go_source_code', doc: 'Go source code string' }], returns: 'foreign_obj', example: 'import go\ngo_mod := go.compile("\n//export Add\nfunc Add(a, b float64) float64 {\n    return a + b\n}\n")\nprintln(go_mod.Add(10.5, 20.5))' },
            'load': { name: 'go.load', signature: 'go.load(so_path_or_pkg)', description: 'Loads a pre-compiled Go shared library (.so) or Go standard package into Skylang.', params: [{ name: 'so_path_or_pkg', doc: 'Path to .so library or Go package name' }], returns: 'foreign_obj', example: 'import go\nlib := go.load("./libcalc.so")' },
            'exec': { name: 'go.exec', signature: 'go.exec(go_source_code)', description: 'Executes Go source code in the Go runtime environment.', params: [{ name: 'go_source_code', doc: 'Go code block' }], returns: 'none', example: 'import go\ngo.exec("println(\\"Hello from Go\\")")' }
        }
    },
    'golang': {
        name: 'golang (Alias for Go Bridge)',
        description: 'Alias for the `go` interoperability module.',
        functions: {
            'compile': { name: 'golang.compile', signature: 'golang.compile(go_source_code)', description: 'JIT-compiles Go source code.', params: [{ name: 'go_source_code', doc: 'Go source code string' }], returns: 'foreign_obj' },
            'load': { name: 'golang.load', signature: 'golang.load(so_path_or_pkg)', description: 'Loads a Go package or shared library.', params: [{ name: 'so_path_or_pkg', doc: 'Path or package name' }], returns: 'foreign_obj' },
            'exec': { name: 'golang.exec', signature: 'golang.exec(go_source_code)', description: 'Executes Go source code.', params: [{ name: 'go_source_code', doc: 'Go code block' }], returns: 'none' }
        }
    },
    'rust': {
        name: 'rust (Rust JIT Compilation & Native cdylib Bridge)',
        description: 'On-the-fly Rust compilation via `rustc --crate-type cdylib` and dynamic linking (`dlopen`) with support for `pub extern "C"` functions.',
        functions: {
            'compile': { name: 'rust.compile', signature: 'rust.compile(rust_source_code)', description: 'JIT-compiles Rust source code with `#[no_mangle] pub extern "C"` bindings on-the-fly and loads the resulting shared library into a callable module.', params: [{ name: 'rust_source_code', doc: 'Rust source code string' }], returns: 'foreign_obj', example: 'import rust\nrs_mod := rust.compile("\n#[no_mangle]\npub extern \\"C\\" fn add(a: f64, b: f64) -> f64 {\n    a + b\n}\n")\nprintln(rs_mod.add(10.5, 20.5))' },
            'load': { name: 'rust.load', signature: 'rust.load(so_path_or_module)', description: 'Loads a pre-compiled Rust cdylib library (.so) or standard module into Skylang.', params: [{ name: 'so_path_or_module', doc: 'Path to .so library or Rust module' }], returns: 'foreign_obj', example: 'import rust\nlib := rust.load("./librustcalc.so")' },
            'exec': { name: 'rust.exec', signature: 'rust.exec(rust_source_code)', description: 'Executes Rust source code.', params: [{ name: 'rust_source_code', doc: 'Rust code block' }], returns: 'none', example: 'import rust\nrust.exec("println!(\\"Hello from Rust\\");")' }
        }
    }
};

export const COMMON_PYTHON_PACKAGES = [
    'numpy', 'pandas', 'torch', 'scipy', 'matplotlib', 'sklearn', 'requests',
    'sys', 'os', 'math', 'json', 'random', 're', 'datetime', 'collections',
    'itertools', 'functools', 'typing', 'csv', 'pathlib', 'http', 'urllib',
    'sqlite3', 'threading', 'multiprocessing', 'subprocess', 'shutil', 'pickle',
    'glob', 'hashlib', 'logging', 'time', 'socket', 'ctypes', 'io', 'base64',
    'asyncio', 'tensorflow', 'flask', 'fastapi', 'django', 'bs4', 'PIL', 'cv2',
    'polars', 'transformers', 'openai', 'anthropic', 'rich', 'tqdm', 'click',
    'typer', 'yaml', 'pydantic', 'pytest', 'seaborn'
];

export const COMMON_JAVA_CLASSES = [
    'java.lang.Math', 'java.lang.String', 'java.lang.System', 'java.lang.Integer',
    'java.lang.Double', 'java.lang.Boolean', 'java.util.ArrayList', 'java.util.HashMap',
    'java.util.HashSet', 'java.util.Arrays', 'java.util.Collections', 'java.util.Random',
    'java.util.Scanner', 'java.io.File', 'java.time.LocalDate', 'java.time.LocalDateTime',
    'java.net.URI', 'java.net.http.HttpClient'
];

export const COMMON_NPM_PACKAGES = [
    'lodash', 'axios', 'express', 'moment', 'chalk', 'fs', 'path', 'http',
    'crypto', 'dotenv', 'rxjs', 'ws', 'dayjs', 'zod', 'commander', 'glob',
    'fs-extra', 'cheerio', 'debug', 'yargs', 'winston', 'bcrypt', 'cors', 'jsonwebtoken'
];

export const COMMON_C_HEADERS = [
    'math.h', 'stdio.h', 'stdlib.h', 'string.h', 'time.h', 'unistd.h',
    'ctype.h', 'limits.h', 'float.h', 'stdbool.h', 'stdint.h', 'errno.h',
    'assert.h', 'sys/time.h', 'sys/stat.h', 'fcntl.h', 'pthread.h'
];

export const COMMON_EXTERN_C_FUNCTIONS: Record<string, { signature: string; header: string; doc: string }> = {
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

export const COMMON_GO_PACKAGES = [
    'fmt', 'math', 'os', 'strings', 'strconv', 'time', 'io', 'net/http',
    'encoding/json', 'sync', 'bytes', 'bufio', 'sort', 'path/filepath', 'crypto/sha256'
];

export const COMMON_RUST_CRATES = [
    'std::f64::consts', 'std::collections', 'std::fs', 'std::io', 'std::time',
    'serde', 'serde_json', 'tokio', 'rayon', 'regex', 'rand', 'chrono'
];
