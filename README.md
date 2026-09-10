<p align="center">
  <img src="assets/skylang_logo.jpg" alt="Skylang Logo" width="220" style="border-radius: 16px;" />
</p>

<h1 align="center">Skylang Reference Manual & Specification</h1>

<p align="center">
  <strong>Comprehensive language specification, formal syntax grammar, and complete API reference for every built-in function, method, property, and module in Skylang.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg" alt="License: Apache 2.0" />
  <img src="https://img.shields.io/badge/build%20%26%20test-passing%20(24%2F24)-brightgreen.svg" alt="Build & Test" />
  <img src="https://img.shields.io/badge/target-C11%20%7C%20GCC%20--O2-blue.svg" alt="C11 & GCC" />
  <img src="https://img.shields.io/badge/GC-Boehm--Demers--Weiser-orange.svg" alt="Memory Management" />
  <img src="https://img.shields.io/badge/interop-Python%20%7C%20JS%20%7C%20C%2B%2B%20%7C%20Java%20%7C%20Go%20%7C%20Rust-purple.svg" alt="Multi-Language" />
</p>

---

## 📑 Table of Contents

1. [Language Grammar & Core Syntax Specification](#1-language-grammar--core-syntax-specification)
   - [Lexical Structure & Keywords](#lexical-structure--keywords)
   - [Variable Declarations & Assignment](#variable-declarations--assignment)
   - [Operators & Precedence](#operators--precedence)
   - [Control Flow Statements](#control-flow-statements)
   - [Function Declarations & Parameter Gateway](#function-declarations--parameter-gateway)
   - [Async Functions, Await & Spawn Syntax](#async-functions-await--spawn-syntax)
   - [Class & Object-Oriented Syntax](#class--object-oriented-syntax)
   - [Module Imports & Export Resolution](#module-imports--export-resolution)
   - [Foreign Function Interface (C FFI) Syntax](#foreign-function-interface-c-ffi-syntax)
   - [F-String Interpolation Syntax](#f-string-interpolation-syntax)
   - [Comments](#comments)
2. [Data Types & Memory Model](#2-data-types--memory-model)
   - [Type Overview](#type-overview)
   - [Default Initial Values](#default-initial-values)
   - [The `.T` Type Property](#the-t-type-property)
3. [Global Built-in Functions](#3-global-built-in-functions)
4. [String Methods & Properties](#4-string-methods--properties)
5. [Collection Methods & Properties](#5-collection-methods--properties)
   - [Array (`array`)](#array-methods--properties)
   - [List (`list`)](#list-methods--properties)
   - [Tuple (`tuple`)](#tuple-methods--properties)
   - [Dictionary (`dict`)](#dictionary-methods--properties)
   - [Set (`set`)](#set-methods--properties)
   - [Sorted List (`sortedList`)](#sorted-list-methods--properties)
6. [File Object Methods & Properties](#6-file-object-methods--properties)
7. [Standard Library Modules](#7-standard-library-modules)
   - [The `math` Module](#the-math-module)
   - [The `str` Module](#the-str-module)
   - [The `async` Concurrency Module](#the-async-concurrency-module)
8. [Multi-Language Interoperability Bridges](#8-multi-language-interoperability-bridges)
   - [Python Bridge (`python`)](#python-bridge)
   - [JavaScript / NPM Bridge (`js`)](#javascript--npm-bridge)
   - [C++ JIT Bridge (`cpp`)](#c-jit-bridge)
   - [Java Bridge (`java`)](#java-bridge)
   - [Golang Bridge (`go`, `golang`)](#golang-bridge)
   - [Rust Bridge (`rust`)](#rust-bridge)
9. [Error Handling & Runtime Exceptions](#9-error-handling--runtime-exceptions)
10. [CLI Toolchain Commands & Multi-Platform Installation](#10-cli-toolchain-commands--multi-platform-installation)
11. [VS Code Extension Reference & Foreign IntelliSense](#11-vs-code-extension-reference--foreign-intellisense)
12. [License](#12-license)

---

## 1. Language Grammar & Core Syntax Specification

### Lexical Structure & Keywords

Skylang reserves the following keywords:

```
async       await       break       cimport     class       continue    
DICT        elif        else        extern      f           false       
for         from        go          golang      if          import      
in          init        js          L           new         nil         
none        None        panic       python      return      rust        
SET         SL          spawn       T           takes       this        
true        while       I           D           B           C           S
```

### Variable Declarations & Assignment

#### 1. Inferred Declaration (Walrus Operator)
`identifier := expression`
- Automatically allocates storage and infers data type from the evaluated expression.

#### 2. Explicit Scalar Declarations
- `I <identifier> [= <expr>]`: Signed 64-bit integer.
- `D <identifier> [= <expr>]`: 64-bit double precision float.
- `B <identifier> [= <expr>]`: Boolean (`true` or `false`).
- `C <identifier> [= <expr>]`: 8-bit character literal.
- `S <identifier> [= <expr>]`: Heap-allocated UTF-8 string.

#### 3. Explicit Collection Declarations
- `<identifier> <TYPE> <SIZE>`: Fixed contiguous array of fixed size.
- `<identifier> L`: Resizable dynamic list.
- `<identifier> T`: Fixed tuple sequence.
- `<identifier> DICT`: Key-value hash map.
- `<identifier> SET`: Hash set of unique values.
- `<identifier> SL`: Self-balancing sorted list.

#### 4. Assignment & Compound Mutators
- `=` : Direct assignment (`x = 10`)
- `+=` : Addition / concatenation assignment
- `-=` : Subtraction assignment
- `*=` : Multiplication assignment
- `/=` : Division assignment

---

### Operators & Precedence

Listed from highest to lowest precedence:

| Precedence | Operator | Description | Associativity |
|---|---|---|---|
| 1 | `.` `[]` `()` | Member access, Indexing / Slicing, Function call | Left-to-right |
| 2 | `^` | Exponentiation / Power | Right-to-left |
| 3 | `!` `-` `+` `await` `spawn` | Logical NOT, Unary negation, Unary positive, Async operators | Right-to-left |
| 4 | `*` `/` `//` `%` | Multiplication, Division, Floor division, Modulo | Left-to-right |
| 5 | `+` `-` | Addition / Concatenation, Subtraction | Left-to-right |
| 6 | `<` `<=` `>` `>=` | Relational ordering comparisons | Left-to-right |
| 7 | `==` `!=` | Equality and inequality | Left-to-right |
| 8 | `and` `&&` | Logical conjunction (short-circuiting) | Left-to-right |
| 9 | `or` `\|\|` | Logical disjunction (short-circuiting) | Left-to-right |
| 10 | `:=` `=` `+=` `-=` `*=` `/=` | Variable binding and assignment mutators | Right-to-left |

---

### Control Flow Statements

#### `if` / `elif` / `else`
```
if <condition> {
    <statements>
} elif <condition> {
    <statements>
} else {
    <statements>
}
```
- Condition expression does not require enclosing parentheses.
- Curly braces `{}` are mandatory.

#### `for ... in` Loop
```
for <identifier> in <iterable_expression> {
    <statements>
}
```
- Iterates across lists, arrays, tuples, sets, sorted lists, string characters, or `range(...)` sequences.

#### `while` Loop
```
while <condition> {
    <statements>
}
```
- Executes repeatedly while `<condition>` evaluates to truthy.

#### Loop Jumps
- `break`: Terminates innermost loop execution immediately.
- `continue`: Skips remainder of current iteration and proceeds to next loop cycle.

---

### Function Declarations & Parameter Gateway

#### Bracketless Function Syntax
```
f <function_name> {
    [takes(<param1>, <param2>, ...)]
    <statements>
    [return <expr1>, <expr2>, ...]
}
```

#### Parameter Gateway (`takes`)
- `takes(p1, p2, ...)`: Validates positional arguments and enables named argument dispatch (`func(p2=val2, p1=val1)`).
- Omission of `takes(...)` makes the function variadic by default via implicit `args` list identifier.

#### Multiple Return Values & Unpacking
- Functions can return multiple expressions separated by commas: `return val1, val2`.
- Caller unbinds with comma-separated assignment: `a, b := fn()`.
- Blank identifier `_` discards unwanted return slots: `val, _ := fn()`.

---

### Async Functions, Await & Spawn Syntax

#### 1. Async Function Declarations (`async f`)
Prefixing a function declaration with `async` defines an asynchronous function that executes in the background:
```skylang
async f fetch_user {
    takes(user_id)
    await async.sleep(0.05) // non-blocking pause
    return {"id": user_id, "name": "Alice"}
}
```

#### 2. The `await` Expression
Waits for an asynchronous task to finish and returns its response value directly:
```skylang
user := await fetch_user(42)
println(user["name"]) // "Alice"
```

#### 3. The `spawn` Task Expression
Converts **any regular synchronous function** into a concurrent task executing on a background worker thread:
```skylang
// 1. Run in background and await response directly:
result := await spawn compute_heavy_task(1000)

// 2. Or launch background worker and retrieve response when needed:
worker := spawn compute_heavy_task(1000)
// ... do other work ...
result := await worker
```

---

### Class & Object-Oriented Syntax

```
class <ClassName> {
    this.<field1>
    this.<field2>

    init {
        takes(<param1>, <param2>, ...)
        this.<field1> = <param1>
    }

    <method_name> {
        [takes(<param1>, ...)]
        <statements>
    }
}
```
- `this.<field>`: Declares instance fields (encapsulated and private by default).
- `init`: Mandatory constructor entry point invoked upon instantiation `ClassName(...)` or `new ClassName(...)`.
- Methods are declared bracketless inside class body without the `f` keyword prefix.

---

### Module Imports & Export Resolution

| Syntax | Description |
|---|---|
| `import <module_name>` | Imports module into current namespace accessible via `<module_name>.<symbol>` |
| `import <module_name> as <alias>` | Imports module with renamed namespace prefix `<alias>` |
| `import <mod1>, <mod2>, <mod3>` | Multi-module single-line import statement |
| `from <module_name> import <sym1>, <sym2>` | Selectively imports specific functions, classes, or constants directly into local scope |
| `from <module_name> import <sym> as <alias>` | Selectively imports symbol with a local alias name |
| `from <module_name> import *` | Imports all top-level public definitions into local scope |
| `from <dir>.<submod> import <sym>` | Resolves nested directory submodules (`dir/submod.sky`) |

---

### Foreign Function Interface (C FFI) Syntax

- `cimport "<header_name.h>"`: Injects native C preprocessor `#include <header_name.h>` into transpilation unit.
- `extern f <function_name>(<param_types>)`: Declares direct linkable C symbol signature.

---

### F-String Interpolation Syntax

- Delimiters: `f"..."`, `f'...'`, `F"..."`, `F'...'`
- Embedded expressions: `{<expression>}` (evaluates any valid Skylang expression, collection access, or function call).
- Literal brace escapes: `{{` outputs `{`, `}}` outputs `}`.
- Escaped quotes within embedded expressions: supports both `\"` and `\'`.

---

### Comments

- `// <comment>` : Single-line C-style comment
- `# <comment>` : Single-line Python-style comment
- `/* <comment> */` : Multi-line block comment

---

## 2. Data Types & Memory Model

### Type Overview

| Type Identifier | Description | Internal Structure |
|---|---|---|
| `int` | Signed 64-bit integer | `int64_t` |
| `double` | 64-bit IEEE 754 floating point | `double` |
| `bool` | Boolean flag (`true` or `false`) | `bool` (`1` / `0`) |
| `char` | 8-bit character | `char` |
| `string` | Immutable UTF-8 string | `char*` with cached length |
| `type` | Type reflection descriptor | `const char*` type tag |
| `array` | Contiguous homogeneous fixed memory | `ValueArray` |
| `list` | Resizable dynamic heterogeneous sequence | `ValueList` |
| `tuple` | Immutable fixed sequence | `ValueTuple` |
| `dict` | Key-value open-addressing hash table | `ValueDict` |
| `set` | Unique element hash set | `ValueSet` |
| `sortedList` | Auto-sorting insertion sequence | `ValueSortedList` |

### Default Initial Values

| Uninitialized Declaration | Auto-Assigned Default Value |
|---|---|
| `I var` | `0` |
| `D var` | `0.0` |
| `B var` | `false` |
| `C var` | `'a'` |
| `S var` | `""` (empty string) |
| `arr I 5` | `<0, 0, 0, 0, 0>` |
| `lst L` | `[]` (empty list) |
| `dict DICT` | `{}` (empty dictionary) |
| `st SET` | `{}` (empty set) |
| `sl SL` | `[]` (empty sorted list) |

### The `.T` Type Property

- `<value>.T`: Returns the `type` representation of the target value.
- Supported string comparison values: `"int"`, `"double"`, `"bool"`, `"char"`, `"string"`, `"array"`, `"list"`, `"tuple"`, `"dict"`, `"set"`, `"sortedList"`, `"type"`, `"file"`, `"foreign"`.

---

## 3. Global Built-in Functions

| Function Signature | Return Type | Description |
|---|---|---|
| `print(...args)` | `none` | Outputs stringified arguments to standard output without trailing newline. |
| `println(...args)` | `none` | Outputs stringified arguments to standard output followed by a newline. |
| `input([prompt: string])` | `string` | Displays optional prompt and reads a single line from standard input. |
| `open(path: string, [mode: string])` | `file` | Opens file handle with mode (`"r"`, `"w"`, `"a"`, `"r+"`, `"w+"`, `"a+"`). |
| `range(stop: int)` | `list` | Generates integer list `[0, 1, ..., stop - 1]`. |
| `range(start: int, stop: int, [step: int])` | `list` | Generates integer list from `start` up to `stop` with stride `step`. |
| `len(container)` | `int` | Returns number of elements in string, list, array, tuple, dict, or set. |
| `type(value)` | `type` | Returns reflection type object descriptor of target value. |
| `hex(number: int)` | `string` | Converts integer to lowercase hexadecimal string representation. |
| `bin(number: int)` | `string` | Converts integer to binary string representation. |
| `oct(number: int)` | `string` | Converts integer to octal string representation. |
| `format(template: string, ...args)` | `string` | Interpolates `{}` positional placeholders in template with arguments. |
| `upper(s: string)` | `string` | Converts all characters in string to uppercase. |
| `lower(s: string)` | `string` | Converts all characters in string to lowercase. |
| `trim(s: string)` | `string` | Strips leading and trailing whitespace characters. |
| `trimleft(s: string)` | `string` | Strips leading whitespace characters. |
| `trimright(s: string)` | `string` | Strips trailing whitespace characters. |
| `split(s: string, [delimiter: string])` | `list` | Splits string by delimiter into a list of strings (splits by chars if omitted). |
| `join(items: list, delimiter: string)` | `string` | Concatenates list elements using delimiter separator. |
| `replace(s: string, old: string, new: string)`| `string` | Replaces all occurrences of `old` substring with `new` substring. |
| `startswith(s: string, prefix: string)` | `bool` | Returns `true` if string begins with `prefix`, else `false`. |
| `endswith(s: string, suffix: string)` | `bool` | Returns `true` if string terminates with `suffix`, else `false`. |
| `contains(s: string, substring: string)` | `bool` | Returns `true` if `substring` exists within target string. |
| `find(s: string, substring: string)` | `int` | Returns 0-based start index of first occurrence of `substring`, or `-1`. |
| `count(s: string, substring: string)` | `int` | Returns frequency count of occurrences of `substring` in string. |
| `reverse(s: string)` | `string` | Returns a reversed copy of the string. |
| `chars(s: string)` | `list` | Returns list of single-character strings extracted from target string. |
| `bytes(s: string)` | `list` | Returns list of integer ASCII byte values for each character. |
| `sorted(container: list)` | `list` | Returns a new ascending-sorted copy of input list. |
| `reversed(container: list)` | `list` | Returns a new reversed copy of input list. |
| `min(...args)` | `scalar` | Evaluates arguments or list and returns smallest value. |
| `max(...args)` | `scalar` | Evaluates arguments or list and returns largest value. |
| `sum(items: list)` | `number` | Calculates total arithmetic sum of all numeric items in list. |
| `abs(n: number)` | `number` | Returns absolute non-negative numeric value. |
| `sqrt(n: number)` | `double` | Calculates square root ($\sqrt{n}$). |
| `error(message: string)` | `error` | Constructs an explicit error value for Go-style error returns. |
| `panic(message: string)` | `void` | Halts execution immediately and displays runtime stack traceback. |
| `eval(code: string)` | `Value` | Compiles and evaluates dynamic Skylang expression/statement at runtime. |
| `gc()` | `void` | Triggers an immediate full cycle of Boehm Garbage Collector. |
| `free(obj: Object)` | `void` | Explicitly deallocates heap-allocated object instance. |

---

## 4. String Methods & Properties

### String Properties
- `s.size`: Returns integer character count.
- `s.length`: Alias for `s.size`.
- `s.chars`: Returns dynamic list of single-character strings.
- `s.bytes`: Returns dynamic list of ASCII integer values.
- `s.T`: Returns `<type string>`.

### String Indexing & Slicing
- `s[i]`: Accesses 0-based character at index `i`.
- `s[start:end]`: Returns substring slice from index `start` up to `end - 1`.
- `s[start:]`: Substring slice from `start` through end of string.
- `s[:end]`: Substring slice from index `0` up to `end - 1`.
- `s[-k]`: Negative indexing accessing $k$-th character from end.

### String Methods

| Method Signature | Return Type | Description |
|---|---|---|
| `s.value(index: int)` | `char` | Retrieves character value at 0-based index. |
| `s.upper()` | `string` | Returns copy with all characters converted to uppercase. |
| `s.lower()` | `string` | Returns copy with all characters converted to lowercase. |
| `s.trim()` | `string` | Returns copy with leading/trailing whitespace removed. |
| `s.trimleft()` | `string` | Returns copy with leading whitespace removed. |
| `s.trimright()` | `string` | Returns copy with trailing whitespace removed. |
| `s.split([delimiter: string])` | `list` | Splits string by delimiter (or characters if omitted). |
| `s.join(items: list)` | `string` | Joins list elements using `s` as delimiter. |
| `s.replace(old: string, new: string)`| `string` | Replaces occurrences of `old` with `new`. |
| `s.startswith(prefix: string)` | `bool` | Checks prefix matching. |
| `s.endswith(suffix: string)` | `bool` | Checks suffix matching. |
| `s.contains(substr: string)` | `bool` | Tests substring existence. |
| `s.has(substr: string)` | `bool` | Alias for `.contains()`. |
| `s.find(substr: string)` | `int` | Returns index of first match or `-1`. |
| `s.count(substr: string)` | `int` | Counts total non-overlapping occurrences of substring. |
| `s.reverse()` | `string` | Returns reversed string. |
| `s.format(...args)` | `string` | Interpolates `{}` placeholders with arguments. |
| `s.rjust(width: int, [fill: string])`| `string` | Right-justifies string in field of minimum `width`. |
| `s.ljust(width: int, [fill: string])`| `string` | Left-justifies string in field of minimum `width`. |
| `s.center(width: int, [fill: string])`| `string`| Centers string in field of minimum `width`. |
| `s.zfill(width: int)` | `string` | Pads numeric string with leading zeros to match `width`. |

---

## 5. Collection Methods & Properties

### Array Methods & Properties (`array`)
- `arr.size`: Integer element count ($O(1)$ header lookup).
- `arr.length`: Alias for `arr.size`.
- `arr.T`: Returns `<type array>`.
- `arr[i]`: Accesses/mutates element at index `i`.
- `arr[start:end]`: Returns sub-array slice.

---

### List Methods & Properties (`list`)
- `l.size`: Integer count of elements ($O(1)$).
- `l.length`: Alias for `l.size`.
- `l.T`: Returns `<type list>`.
- `l[i]`: Accesses/mutates element at index `i`.
- `l[start:end]`: Returns sub-list slice.

| List Method Signature | Return Type | Description |
|---|---|---|
| `l.push(value)` | `void` | Appends `value` to tail of list ($O(1)$ amortized). |
| `l.pop([index: int])` | `Value` | Removes and returns element at `index` (defaults to last element). |
| `l.insert(index: int, value)` | `void` | Inserts `value` at specified `index`. |
| `l.remove(value)` | `bool` | Removes first occurrence of `value` from list. |
| `l.clear()` | `void` | Empties all elements from list. |
| `l.contains(value)` / `l.has(value)` | `bool` | Returns `true` if `value` exists in list. |
| `l.index(value)` | `int` | Returns 0-based index of first occurrence of `value` or `-1`. |
| `l.count(value)` | `int` | Returns total occurrences of `value` in list. |
| `l.reverse()` | `void` | Inverts list elements in-place. |
| `l.sort()` | `void` | Sorts list elements in ascending order in-place. |
| `l.join(delimiter: string)` | `string` | Concatenates list elements into string separated by `delimiter`. |
| `l.copy()` | `list` | Returns a shallow clone of list. |

---

### Tuple Methods & Properties (`tuple`)
- `t.size`: Integer element count.
- `t.length`: Alias for `t.size`.
- `t.T`: Returns `<type tuple>`.
- `t[i]`: Accesses element at index `i` (immutable, cannot assign).
- `t[start:end]`: Sub-tuple slice.

---

### Dictionary Methods & Properties (`dict`)
- `d.size`: Integer key-value pair count.
- `d.length`: Alias for `d.size`.
- `d.T`: Returns `<type dict>`.
- `d[key]`: Accesses value associated with `key` (panics if missing).
- `d[key] = val`: Inserts or updates entry.

| Dict Method Signature | Return Type | Description |
|---|---|---|
| `d.has(key)` / `d.contains(key)` | `bool` | Returns `true` if `key` exists in dictionary. |
| `d.get(key, [default_val])` | `Value` | Returns value for `key`, or `default_val` (`none` if omitted). |
| `d.remove(key)` / `d.pop(key)` | `Value` | Deletes key and returns associated value. |
| `d.keys()` | `list` | Returns dynamic list of all keys. |
| `d.values()` | `list` | Returns dynamic list of all values. |
| `d.items()` | `list` | Returns list of `(key, value)` tuple pairs. |
| `d.clear()` | `void` | Removes all key-value entries. |
| `d.copy()` | `dict` | Returns a shallow copy of dictionary. |
| `d.update(other_dict)` | `void` | Merges entries from `other_dict` into `d`. |

---

### Set Methods & Properties (`set`)
- `s.size`: Unique element count.
- `s.length`: Alias for `s.size`.
- `s.T`: Returns `<type set>`.

| Set Method Signature | Return Type | Description |
|---|---|---|
| `s.add(value)` | `void` | Inserts `value` into set (no-op if duplicate). |
| `s.remove(value)` | `bool` | Removes `value` from set. |
| `s.has(value)` / `s.contains(value)` | `bool` | Tests element membership. |
| `s.clear()` | `void` | Removes all elements from set. |
| `s.union(other_set)` | `set` | Returns new set with elements from both sets ($A \cup B$). |
| `s.intersect(other_set)` | `set` | Returns new set with shared elements ($A \cap B$). |
| `s.diff(other_set)` | `set` | Returns new set with elements in `s` but not `other_set` ($A \setminus B$). |

---

### Sorted List Methods & Properties (`sortedList`)
- `sl.size`: Element count.
- `sl.length`: Alias for `sl.size`.
- `sl.T`: Returns `<type sortedList>`.

| SortedList Method Signature | Return Type | Description |
|---|---|---|
| `sl.add(value)` | `void` | Inserts `value` into maintaining sorted order ($O(\log N)$). |
| `sl.remove(value)` | `bool` | Removes first occurrence of `value`. |
| `sl.pop()` | `Value` | Removes and returns highest element. |
| `sl.has(value)` | `bool` | Tests membership via binary search. |
| `sl.clear()` | `void` | Empties sorted list. |

---

## 6. File Object Methods & Properties

Returned by `open(filepath, mode)`:

### File Properties
- `f.is_open`: Boolean flag indicating if stream is active.
- `f.mode`: Mode string (`"r"`, `"w"`, `"a"`, etc.).
- `f.path`: Absolute or relative file path string.
- `f.T`: Returns `<type file>`.

### File Methods
| File Method Signature | Return Type | Description |
|---|---|---|
| `f.read([bytes_count: int])` | `string` | Reads entire file content (or up to `bytes_count`). |
| `f.write(data: string)` | `int` | Writes string data to file buffer. |
| `f.readline()` | `string` | Reads next line including newline delimiter. |
| `f.readlines()` | `list` | Reads all remaining lines into a list of strings. |
| `f.flush()` | `void` | Flushes stream write buffer to disk. |
| `f.close()` | `void` | Flushes buffer and closes underlying OS file descriptor. |

---

## 7. Standard Library Modules

### The `math` Module

#### Functions
- `math.sqrt(x: number)` : Square root ($\sqrt{x}$)
- `math.sin(x: number)` : Trigonometric sine of radians
- `math.cos(x: number)` : Trigonometric cosine of radians
- `math.tan(x: number)` : Trigonometric tangent of radians
- `math.asin(x: number)` : Arc sine
- `math.acos(x: number)` : Arc cosine
- `math.atan(x: number)` : Arc tangent
- `math.atan2(y: number, x: number)` : Two-argument arc tangent
- `math.abs(x: number)` : Absolute value
- `math.ceil(x: number)` : Ceiling rounding
- `math.floor(x: number)` : Floor rounding
- `math.round(x: number)` : Nearest integer rounding
- `math.log(x: number)` : Natural logarithm ($\ln x$)
- `math.log10(x: number)` : Base-10 logarithm
- `math.log2(x: number)` : Base-2 logarithm
- `math.exp(x: number)` : Exponential function ($e^x$)
- `math.pow(base: number, exp: number)` : Power ($base^{exp}$)
- `math.min(a: number, b: number)` : Minimum of two numbers
- `math.max(a: number, b: number)` : Maximum of two numbers
- `math.hypot(x: number, y: number)` : Hypotenuse length ($\sqrt{x^2 + y^2}$)
- `math.clamp(val: number, min: number, max: number)` : Clamps value within bounds
- `math.deg2rad(degrees: number)` : Converts degrees to radians
- `math.rad2deg(radians: number)` : Converts radians to degrees

#### Constants
- `math.pi`: Mathematical constant $\pi \approx 3.141592653589793$
- `math.e`: Euler's number $e \approx 2.718281828459045$
- `math.inf`: Positive infinity ($+\infty$)
- `math.nan`: IEEE Not-a-Number representation

---

### The `str` Module

- `str.upper(s)`: Uppercase transform
- `str.lower(s)`: Lowercase transform
- `str.trim(s)`: Leading/trailing whitespace strip
- `str.trimleft(s)`: Leading whitespace strip
- `str.trimright(s)`: Trailing whitespace strip
- `str.split(s, [delim])`: String split
- `str.join(delim, list)`: List join
- `str.replace(s, old, new)`: Substring replace
- `str.startswith(s, prefix)`: Prefix test
- `str.endswith(s, suffix)`: Suffix test
- `str.contains(s, sub)`: Substring test
- `str.find(s, sub)`: Substring search index
- `str.count(s, sub)`: Substring frequency
- `str.reverse(s)`: String reversal
- `str.chars(s)`: Character list extraction
- `str.bytes(s)`: Byte value list extraction
- `str.format(fmt, ...args)`: Formatted placeholder template

---

### The `async` Concurrency Module

- `async.sleep(seconds: number)`: Pauses execution for `seconds` without blocking the main event loop (use with `await`).
- `async.all(tasks: list)`: Runs multiple background tasks in parallel and returns a list containing each task's resolved return value.
- `async.race(tasks: list)`: Waits for multiple background tasks in parallel and returns the resolved result of whichever task completes first.
- `async.spawn(callee: function, ...args)`: Spawns any synchronous function with arguments in a background worker task.

---

## 8. Multi-Language Interoperability Bridges

### Python Bridge (`python`)
- `python.load(module_name: string)`: Loads Python C-API module into Skylang object.
- `python.exec(python_code: string)`: Executes arbitrary Python code block.

### JavaScript / NPM Bridge (`js`)
- `js.load(module_or_package: string)`: Loads Node.js / NPM package.
- `js.exec(js_code: string)`: Executes JavaScript code via Node runtime.

### C++ JIT Bridge (`cpp`)
- `cpp.compile(cpp_source: string)`: JIT compiles C++ source with GCC into shared object.
- `cpp.load(shared_library_path: string)`: Loads native C++ dynamic library.

### Java Bridge (`java`)
- `java.load(class_name: string)`: Loads Java class via reflection.
- `java.exec(method_call: string)`: Dispatches static/instance JVM method.

### Golang Bridge (`go`, `golang`)
- `go.load(package_name: string)`: Loads Go runtime package.
- `go.compile(go_source: string)`: JIT compiles Go code into `c-shared` binary bridge.
- `go.exec(function_call: string)`: Dispatches Go exported function.

### Rust Bridge (`rust`)
- `rust.load(crate_name: string)`: Loads Rust crate library.
- `rust.compile(rust_source: string)`: JIT compiles Rust code into `cdylib` bridge.
- `rust.exec(function_call: string)`: Dispatches Rust `extern "C"` function.

---

## 9. Error Handling & Runtime Exceptions

### Error Protocol
- `error(msg: string)`: Constructs Go-style error object.
- Functions returning single values set `err` slot to `none` automatically upon multi-unpacking: `res, err := fn()`.
- `panic(msg: string)`: Halts execution immediately and unwinds runtime call stack.

### Runtime Exceptions Hierarchy
- `SyntaxError`: Parser encountered unexpected tokens or malformed constructs.
- `NameError`: Reference to an undeclared identifier or symbol.
- `TypeError`: Incompatible operands for operator or mismatched parameter arity.
- `ZeroDivisionError`: Division or floor division by zero (`/`, `//`, `%`).
- `IndexError`: Array, list, tuple, or slice index outside container bounds.
- `AttributeError`: Member field or method does not exist on target object.
- `ModuleNotFoundError`: Import resolution failed to locate `.sky` file or native bridge.
- `FileNotFoundError`: File path does not exist during I/O operation.
- `PermissionError`: Operating system access permission denied.

---

## 10. CLI Toolchain Commands & Multi-Platform Installation

### Cross-Platform Installation

| Platform | Installer Command | Details |
|---|---|---|
| **Windows (.exe Installer)** | `skylang-installer.exe` | Native standalone Win32 installer: configures `%LOCALAPPDATA%\Skylang`, adds `bin\` to Registry PATH, registers `.sky` file associations, creates `uninstall.exe` |
| **Linux & macOS** | `./install.sh` | Detects Homebrew/apt/dnf/pacman, builds and links `sky` and `skylang` to `/usr/local/bin` |
| **From Source** | `make clean all` | Builds `bin/skylang`, `bin/sky`, and `lib/libskylang_rt.a` via Makefile / MinGW |

#### Windows Installer CLI Options
```cmd
skylang-installer.exe             :: Interactive installation wizard
skylang-installer.exe --silent    :: Unattended / silent installation
skylang-installer.exe --dir "C:\Skylang" :: Custom destination path
skylang-installer.exe --uninstall :: Remove Skylang and registry associations
```

### Uninstallation
- **Windows**: Run `uninstall.exe` from your Skylang installation folder or Windows *Add or Remove Programs*
- **Linux / macOS**: `./delete.sh`

### CLI Commands

| Command Signature | Function |
|---|---|
| `sky run <file.sky>` | Transpiles, compiles to C, and executes program immediately. |
| `sky build <file.sky> [-o <bin>]` | Compiles into a standalone native C11 executable binary. |
| `sky <file.sky>` | Alias for `sky run <file.sky>`. |
| `make clean all` | Rebuilds compiler, VM, runtime library, and binaries from source. |
| `make test` | Executes comprehensive automated test suite. |
| `make vscode` | Recompiles and updates local VS Code extension installation. |

---

## 11. VS Code Extension Reference & Foreign IntelliSense

- **File Associations**: `.sky`, `.skylang`
- **Grammar Scopes**: TextMate grammar covering bracketless functions, `async f`, `await`, `spawn`, `takes(...)`, walrus `:=`, scalar prefixes, f-strings, and multi-language bridges.
- **Universal Foreign IntelliSense**: Autocompletions, parameter hints, and hover documentation for:
  - **Node.js / JS**: `fs`, `path`, `http`, `express`, `lodash`, `axios`, `Math`, `JSON`
  - **Python 3**: `math`, `sys`, `os`, `json`, `numpy`, `requests`
  - **Java / JVM**: `java.lang.Math`, `java.util.ArrayList`, `java.util.HashMap`, `java.io.File`
  - **Golang**: `fmt`, `math`, `os`, `net/http`, `strings`, `sync`
  - **Rust**: `std::f64::consts`, `std::collections::HashMap`, `std::fs`, `std::io`
  - **C/C++ FFI**: `stdio.h`, `stdlib.h`, `math.h`, `string.h`, `unistd.h`
- **Snippets Catalog**:
  - `asyncf` / `await` / `spawn`: Asynchronous functions, task await, and background thread spawning
  - `asyncsleep` / `asyncall` / `asyncrace` / `asyncspawn`: Concurrency standard library combinators
  - `f` / `fn`: Function declaration with `takes(...)`
  - `fvar`: Variadic function declaration with `args`
  - `takes`: Parameter filtering gateway
  - `class`: OOP Class declaration with `this.` fields and `init`
  - `import` / `impas` / `from` / `fromas` / `fromall`: Module imports
  - `fstr`: Formatted string literal interpolation
  - `cimport` / `extern`: C FFI bindings
  - `forin` / `while`: Loop structures
  - `imppy` / `impjs` / `impcpp` / `impjava` / `goload` / `rustload`: Multi-language imports
  - `openread` / `openwrite` / `openlines`: File operations
  - `errhandle`: Go-style error handling check

---

## 12. License

This project is licensed under the [Apache License, Version 2.0](LICENSE) - see the [LICENSE](LICENSE) file for details.

```text
Copyright 2026 Aakash Dandekar

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
