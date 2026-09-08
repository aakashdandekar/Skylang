<p align="center">
  <img src="assets/skylang_logo.jpg" alt="Skylang Logo" width="220" style="border-radius: 16px;" />
</p>

<h1 align="center">Skylang</h1>

<p align="center">
  <strong>A high-performance compiled, dynamic, object-oriented programming language built upon C.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/build%20%26%20test-passing%20(24%2F24)-brightgreen.svg" alt="Build & Test" />
  <img src="https://img.shields.io/badge/target-C11%20%7C%20GCC%20--O2-blue.svg" alt="C11 & GCC" />
  <img src="https://img.shields.io/badge/GC-Boehm--Demers--Weiser-orange.svg" alt="Memory Management" />
  <img src="https://img.shields.io/badge/interop-Python%20%7C%20JS%20%7C%20C%2B%2B%20%7C%20Java%20%7C%20Go%20%7C%20Rust-purple.svg" alt="Multi-Language" />
</p>

Skylang blends **Python's readability & dynamic ergonomics**, **Go's clean control flow & error patterns**, and **C's native speed** into a cohesive modern language. Programs can be transpiled directly to optimized C11 or executed on an integrated stack-based Bytecode Virtual Machine with profiling.

```skylang
// Hello World & Multi-Language Interop in Skylang
import python, js, go, rust

message := "Hello, Skylang!"
println(message)

// Skylang Built-in Dynamic Evaluation
eval_res := eval("10 * 20 + 56")
println("eval('10 * 20 + 56'):", eval_res)

py_math := python.load("math")
println("Python sqrt(256):", py_math.sqrt(256))

js_math := js.load("Math")
println("JS Math.pow(2, 10):", js_math.pow(2, 10))

go_math := go.load("math")
println("Go math.Sqrt(625.0):", go_math.Sqrt(625.0))

rust_math := rust.load("std::f64::consts")
println("Rust math PI:", rust_math.PI)
```

---

## Table of Contents

- [Installation](#installation)
- [CLI Usage](#cli-usage)
- [Variables & Data Types](#variables--data-types)
- [Operators](#operators)
- [Strings](#strings)
- [Arrays](#arrays)
- [Lists](#lists)
- [Tuples](#tuples)
- [Dictionaries](#dictionaries)
- [Sets](#sets)
- [Sorted Lists](#sorted-lists)
- [Control Flow](#control-flow)
- [Functions](#functions)
  - [Bracketless Declarations](#functions)
  - [Variadic by Default (`args`)](#variadic-by-default--the-args-list)
  - [Named Parameters (`takes`)](#named-parameters-with-takes)
  - [Built-in Functions](#built-in-functions)
- [Classes & OOP](#classes--oop)
- [Modules & Imports (Python-Style)](#modules--imports-python-style)
  - [Standard Module Import (`import mod`)](#1-standard-module-import)
  - [Import with Alias (`as`)](#2-import-with-alias-as)
  - [Selective Symbol Import (`from mod import ...`)](#3-selective-symbol-import-from--import)
  - [Wildcard Import (`from mod import *`)](#4-wildcard-import-from--import-)
  - [Submodules & Subdirectories (`sub.helper`)](#5-submodules--nested-directories)
- [Standard Library](#standard-library)
- [Multi-Language Interoperability (Python, JS/NPM, C++, Java, Go, Rust)](#multi-language-interoperability)
- [Foreign Function Interface (FFI)](#foreign-function-interface-ffi)
- [Dual Execution Architecture (AOT & Bytecode VM)](#dual-execution-architecture)
- [Error Handling & Traceback Engine](#error-handling--traceback-engine)
  - [Go-Style Error Returns](#1-go-style-error-handling)
  - [Unpacking Results & Blank Identifier](#unpacking-results)
  - [Panic — Unrecoverable Errors](#panic--unrecoverable-errors)
  - [Python-Style Traceback Engine](#2-python-style-traceback-engine)
- [Memory Management](#memory-management)
- [Comments](#comments)
- [VS Code Extension & Editor Support](#vs-code-extension--editor-support)
- [Project Structure](#project-structure)
- [Running Tests](#running-tests)
- [Roadmap](#roadmap)
- [License](#license)

---

## Installation

### Prerequisites

| Tool | Purpose |
|------|---------|
| **GCC** (C11+) | C compiler used to compile transpiled output |
| **Make** | Build system |
| **Boehm GC** (`libgc` / `bdw-gc`) | Automatic garbage collection library |
| **pkg-config** | Helps locate the GC library during build |
| **Go / Rust / Python / Node / Java** (Optional) | Required when using respective multi-language interop bridges |

### One-Command Setup

Clone the repo and run the installation script — it handles everything:

```bash
git clone <repo-url> skylang
cd skylang
chmod +x install.sh
./install.sh
```

**What `install.sh` does:**

1. **Detects your OS** and package manager (apt, dnf, pacman, zypper, apk, brew)
2. **Checks for dependencies** — gcc, make, pkg-config, libgc, Python 3, Node.js, Go, Rust, Java
3. **Installs missing packages**
4. **Builds the compiler & runtime** by running `make clean all`
5. **Creates global commands** — symlinks `skylang` and `sky` into `~/.local/bin`
6. **Updates your PATH** — appends `~/.local/bin` to your shell profile if not present
7. **Installs VS Code extension** — automatic syntax highlighting and autocomplete
8. **Runs the test suite** to verify everything works

To uninstall Skylang completely, run:
```bash
./delete.sh
```

After setup completes, restart your terminal (or `source ~/.bashrc`), then:

```bash
sky run examples/01_basics.sky    # works from any directory
```

### Manual Build (if you prefer)

```bash
make clean && make
./bin/sky run examples/01_basics.sky
```

---

## CLI Usage

After installation, two commands are available globally: `skylang` and `sky` (an alias).

| Command | What it does |
|---|---|
| `sky run <file.sky>` | Transpiles to C, compiles, and executes immediately |
| `sky build <file.sky> [-o <output>]` | Transpiles to C and compiles into a standalone native binary (defaults to `./<basename>`, silent on success) |
| `sky <file.sky>` | Shorthand for `sky run <file.sky>` |

### Examples

```bash
# Run a program immediately
sky run my_app.sky

# Or run directly without the 'run' keyword
sky my_app.sky

# Compile to a native standalone binary (produces `./my_app` silently on success)
sky build my_app.sky
./my_app

# Compile with a custom output binary name
sky build my_app.sky -o custom_bin
./custom_bin
```

---

## Variables & Data Types

Skylang supports **12 built-in types** split into two categories:

### Scalar Types

| Prefix | Type | Default Value | Example |
|--------|------|---------------|---------|
| `I` | `int` | `0` | `I count` |
| `D` | `double` | `0.0` | `D temperature` |
| `B` | `bool` | `false` | `B active` |
| `C` | `char` | `'a'` | `C initial` |
| `S` | `string` | `""` | `S name` |

Declare a variable with its type prefix. It's automatically initialized to the default value:

```skylang
I count          // count = 0
D temperature    // temperature = 0.0
B active         // active = false
C initial        // initial = 'a'
S name           // name = ""
```

Assign a value at declaration:

```skylang
I age = 21
D pi = 3.14159
B logged_in = true
C grade = 'A'
S title = "Skylang Guide"
```

### Collection Types

| Suffix / Keyword | Type | Declaration |
|------------------|------|-------------|
| `I <size>` | `array` (fixed, homogeneous) | `arr I 5` |
| `L` | `list` (dynamic, heterogeneous) | `items L` |
| `T` | `tuple` (fixed, immutable) | `point T` |
| `DICT` | `dict` (hash map) | `scores DICT` |
| `SET` | `set` (unique elements) | `tags SET` |
| `SL` | `sortedList` (auto-sorted) | `grades SL` |

Each collection type is covered in detail below.

### The Walrus Operator `:=`

Use `:=` when you want Skylang to **automatically infer the type** from the value:

```skylang
score := 95           // inferred as int
rating := 4.8         // inferred as double
message := "Success"  // inferred as string
items := [1, 2, 3]    // inferred as list
```

### Type Inspection with `.T`

Every variable has a `.T` property that returns its type:

```skylang
I x = 10
print(x.T)            // <type int>
print(x.T == "int")   // true

S name = "Sky"
print(name.T)          // <type string>
```

The return value of `.T` is of the special `type` data type.

---

## Operators

### Arithmetic

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `+` | Addition (or string/list concatenation) | `10 + 5` | `15` |
| `-` | Subtraction (or unary negation) | `10 - 3` | `7` |
| `*` | Multiplication (or string repetition) | `4 * 6` | `24` |
| `/` | Floating-point division | `10.0 / 4.0` | `2.5` |
| `//` | Floor (integer) division | `10 // 3` | `3` |
| `^` | Exponentiation (power) | `2 ^ 4` | `16` |
| `%` | Modulo (remainder) | `10 % 3` | `1` |

### Comparison

| Operator | Meaning |
|----------|---------|
| `==` | Equal to |
| `!=` | Not equal to |
| `<` | Less than |
| `<=` | Less than or equal |
| `>` | Greater than |
| `>=` | Greater than or equal |

### Logical

| Operator | Meaning |
|----------|---------|
| `and` (or `&&`) | Logical AND |
| `or` (or `||`) | Logical OR |
| `!` | Logical NOT |

### Assignment

| Operator | Meaning |
|----------|---------|
| `=` | Assign |
| `:=` | Walrus (declare + assign with type inference) |
| `+=` | Add and assign |
| `-=` | Subtract and assign |
| `*=` | Multiply and assign |
| `/=` | Divide and assign |

```skylang
I x = 10
x += 5    // x is now 15
x *= 2    // x is now 30
```

---

## Strings

Strings are declared with the `S` prefix or inferred via `:=`. They support indexing, slicing, and built-in properties.

```skylang
S lang = "Skylang"
```

### Indexing

Access individual characters by index (0-based):

```skylang
print(lang[0])           // S
print(lang.value(1))     // k
```

### Slicing

Python-style `[start:end]` slicing — returns a substring from `start` up to (but not including) `end`. Negative indices are supported:

```skylang
print(lang[0:3])         // Sky
print(lang[3:])          // lang
print(lang[-4:])         // lang
```

### Properties & Methods

All string operations in Skylang are first-class built-in methods and properties:

| Usage | Returns | Description |
|-------|---------|-------------|
| `s.size` / `s.length` | `int` | Length of the string |
| `s.chars` | `list` | Dynamic list of individual character strings |
| `s.bytes` | `list` | Dynamic list of integer ASCII byte values |
| `s.value(index)` | `char` | Character at the given index |
| `s[start:end]` | `string` | Substring slice |
| `s.upper()` | `string` | Uppercase conversion |
| `s.lower()` | `string` | Lowercase conversion |
| `s.trim()` | `string` | Strip leading and trailing whitespace |
| `s.trimleft()` / `s.trimright()` | `string` | Strip leading / trailing whitespace |
| `s.split([delimiter])` | `list` | Split by delimiter (or characters if omitted) |
| `s.join(list)` | `string` | Join list elements with separator `s` |
| `s.contains(substr)` / `s.has(substr)` | `bool` | Check if substring exists |
| `s.startswith(prefix)` | `bool` | Check prefix match |
| `s.endswith(suffix)` | `bool` | Check suffix match |
| `s.replace(old, new)` | `string` | Replace occurrences of substring |
| `s.find(substr)` | `int` | 0-based index of substring or -1 |
| `s.count(substr)` | `int` | Number of occurrences of substring |
| `s.reverse()` | `string` | Reversed string |

```skylang
S greeting = "  Hello, World!  "
print(greeting.size)          // 17
print(greeting.trim())        // "Hello, World!"
print("skylang".upper())      // "SKYLANG"
print("a,b,c".split(","))     // ["a", "b", "c"]
print(", ".join(["x", "y"]))  // "x, y"
print("banana".replace("a", "o")) // "bonono"
print("abc".chars)            // ["a", "b", "c"]
print("ABC".bytes)            // [65, 66, 67]
```

### Python-Style F-Strings (Formatted String Interpolation)

Skylang supports full Python-style **f-strings** (`f"..."`, `f'...'`, `F"..."`, `F'...'`) for embedded expression interpolation:

- **Variables & Expressions**: Directly embed variables, calculations, or method calls inside `{...}`.
- **Any Data Type**: Integers, doubles, booleans, lists, dictionaries, tuples, and objects are automatically converted to strings.
- **Escaped Braces**: Use `{{` and `}}` to output literal `{` and `}` characters.
- **Nested Quotes**: Supports double or single quotes inside `{...}` expressions (e.g. dict indexing `f"User {user[\"name\"]}"` or `f'User {user["name"]}'`).

```skylang
name := "Alice"
age := 25
score := 98.5

// Basic variable and arithmetic interpolation
println(f"Hello {name}, in 5 years you will be {age + 5}!")
// Output: Hello Alice, in 5 years you will be 30!

// Expressions, method calls, and collections
items := [10, 20, 30]
user := {"role": "Admin", "active": true}
println(f"User {name} ({user[\"role\"]}) has {items.size} items. Active: {user[\"active\"]}")
// Output: User Alice (Admin) has 3 items. Active: true

// Escaped braces
println(f"Literal {{brace}} and evaluated: {10 * 10}")
// Output: Literal {brace} and evaluated: 100

// Single-quoted f-strings
println(f'Single-quoted: {name.upper()}')
// Output: Single-quoted: ALICE
```

---

## Arrays

Arrays are **fixed-size**, **contiguous** blocks of memory holding elements of the **same type**.

### Declaration

```skylang
// Declare an array of 4 ints (all default to 0)
numbers I 4
numbers[0] = 10
numbers[3] = 40
print(numbers)       // <10, 0, 0, 40>

// Array literal with angle brackets
primes I = <2, 3, 5, 7>
print(primes)        // <2, 3, 5, 7>
```

### Properties

| Usage | Returns | Description |
|-------|---------|-------------|
| `arr.size` | `int` | Number of elements (O(1) — stored in memory header) |
| `arr[index]` | element | Access by index |
| `arr[start:end]` | `array` | Slice (Python-style) |

### Array vs List

| Feature | Array | List |
|---------|-------|------|
| Size | Fixed at creation | Dynamic (grows/shrinks) |
| Types | Homogeneous (one type) | Heterogeneous (mixed types) |
| Syntax | `<1, 2, 3>` | `[1, 2, 3]` |
| Memory | Contiguous | Dynamic allocation |
| Access | O(1) random access | O(1) random access |

---

## Lists

Lists are **dynamic**, **resizable** collections that can hold elements of **any type** (heterogeneous). They work like Python lists.

### Declaration

```skylang
// Empty list
tasks L

// List literal
numbers := [10, 20, 30, 40, 50]
```

### Methods & Properties

| Usage | Returns | Description |
|-------|---------|-------------|
| `lst.push(item)` | — | Appends an item to the end |
| `lst.pop()` | element | Removes and returns the last item |
| `lst.pop(index)` | element | Removes and returns the item at `index` |
| `lst.clear()` | — | Removes all items |
| `lst.size` | `int` | Number of items (O(1) — maintained in RAM) |
| `lst[index]` | element | Access by index |
| `lst[start:end]` | `list` | Slice (Python-style) |

```skylang
tasks L
tasks.push("Write code")
tasks.push("Run tests")
tasks.push("Deploy")

print(tasks.size)         // 3
last := tasks.pop()       // "Deploy"
print(tasks)              // ["Write code", "Run tests"]

// Slicing
numbers := [10, 20, 30, 40, 50]
print(numbers[1:4])       // [20, 30, 40]
```

### Performance Note

The `.size` property runs in **O(1)** time — the list length is maintained as a variable in RAM and updated on every push/pop, so accessing size is instant rather than counting elements.

---

## Tuples

Tuples are **fixed-size, immutable** sequences. Once created, their elements cannot be changed.

```skylang
// Declaration
point T = (10, 20, "Origin")

// Access by index
print(point[0])          // 10
print(point[2])          // Origin
print(point.size)        // 3
```

Tuples are useful for returning multiple values from functions and for data that should never change.

---

## Dictionaries

Dictionaries (`dict`) are **key-value hash maps**. Keys can be strings, ints, or other hashable types.

```skylang
// Declaration
scores DICT
scores["Alice"] = 95
scores["Bob"] = 88

// Or with a literal
config := {"host": "localhost", "port": 8080}

// Access
print(scores["Alice"])       // 95
```

### Methods & Properties

| Usage | Returns | Description |
|-------|---------|-------------|
| `dict[key]` | value | Get value by key |
| `dict[key] = val` | — | Set a key-value pair |
| `dict.has(key)` | `bool` | Check if a key exists |
| `dict.size` | `int` | Number of key-value pairs |

```skylang
scores DICT
scores["Alice"] = 95
print(scores.has("Alice"))   // true
print(scores.has("Eve"))     // false
print(scores.size)           // 1
```

---

## Sets

Sets store **unique elements** — duplicates are automatically discarded.

```skylang
tags SET
tags.add("coding")
tags.add("systems")
tags.add("coding")           // duplicate, ignored

print(tags)                  // {systems, coding}
print(tags.size)             // 2
```

### Methods & Properties

| Usage | Returns | Description |
|-------|---------|-------------|
| `set.add(item)` | — | Add an element (ignored if duplicate) |
| `set.has(item)` | `bool` | Check if element exists |
| `set.size` | `int` | Number of unique elements |

---

## Sorted Lists

A `sortedList` **automatically maintains ascending order** upon every insertion. You never need to sort manually.

```skylang
grades SL
grades.push(85)
grades.push(42)
grades.push(99)
grades.push(70)

print(grades)                // SL[42, 70, 85, 99]
```

Elements are inserted in their correct sorted position, so the list is always sorted.

---

## Control Flow

### If / Elif / Else

Curly braces `{}` are required. Parentheses around conditions are optional:

```skylang
I score = 82

if score >= 90 {
    print("Grade: A")
} elif score >= 80 {
    print("Grade: B")
} elif score >= 70 {
    print("Grade: C")
} else {
    print("Grade: F")
}
```

### For Loops (Go-Style Unified Syntax)

Skylang unifies all loops into a single `for` keyword — no `while`, no `do-while`, just `for`:

#### 1. Infinite Loop

Loops forever until `break`:

```skylang
I n = 0
for {
    if n >= 3 {
        break
    }
    print("n:", n)
    n += 1
}
```

#### 2. Condition Loop (replaces `while`)

Loops while the condition is true:

```skylang
I i = 0
for i < 5 {
    print("i is:", i)
    i += 1
}
```

#### 3. Iterator Loop (`for...in`)

Iterates over any collection or string:

```skylang
names := ["Alice", "Bob", "Charlie"]
for name in names {
    print("Hello,", name)
}

S word = "Sky"
for ch in word {
    print("char:", ch)
}
```

### Break & Continue

- `break` — exits the current loop immediately
- `continue` — skips to the next iteration

---

## Functions

Functions are defined with the `f` keyword. **Definitions are bracketless** — no parentheses after the function name:

```skylang
f greet {
    print("Hello from Skylang!")
}

greet()    // Hello from Skylang!
```

### Variadic by Default — The `args` List

By default, every function accepts **any number of arguments**. An implicit `args` list is automatically available inside the function body:

```skylang
f add_all {
    I result
    for i in args {
        result += i
    }
    return result
}

print(add_all(1, 2, 3, 4, 5))    // 15
print(add_all(10, 20))            // 30
```

### Named Parameters with `takes(...)`

When a function needs **specific parameters**, use `takes(...)` as a gateway. It filters incoming arguments and binds them to named variables:

```skylang
f add {
    takes(a, b)
    return a + b
}

// Positional arguments
print(add(3, 4))              // 7

// Named arguments (order doesn't matter)
print(add(a=1, b=2))          // 3
print(add(b=10, a=5))         // 15
```

If a caller passes an argument name not declared in `takes(...)`, Skylang raises a runtime error.

### Return Values

Functions return values with `return`:

```skylang
f square {
    takes(n)
    return n * n
}

result := square(7)    // 49
```

### Recursion

Functions can call themselves:

```skylang
f fibonacci {
    takes(n)
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

print(fibonacci(10))    // 55
```

```skylang
f factorial {
    takes(n)
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

print(factorial(5))     // 120
```

### Built-in Functions

Skylang provides core built-in functions globally without requiring imports:

| Function | Signature | Description |
|----------|-----------|-------------|
| `print(...)` | `print(arg1, arg2, ...)` | Prints arguments space-separated without a trailing newline |
| `println(...)` | `println(arg1, arg2, ...)` | Prints arguments space-separated with a trailing newline |
| `eval(expr)` | `eval(code_str)` | Dynamically evaluates a Skylang expression string at runtime and returns a Value |
| `range(stop)` | `range(stop)` / `range(start, stop, step)` | Returns a list containing an arithmetic sequence |
| `len(coll)` | `len(collection)` | Returns the number of items or string length |
| `type(val)` | `type(val)` | Returns the type descriptor object (equivalent to `val.T`) |
| `takes(...)` | `takes(param1, param2, ...)` | Parameter filtering gateway in functions and methods |
| `error(msg)` | `error(message)` | Constructs an error object for Go-style error returns |
| `panic(msg)` | `panic(message)` | Halts execution immediately and displays the error |
| `gc()` | `gc()` | Triggers an immediate garbage collection cycle |
| `free(obj)` | `free(obj)` | Explicitly frees an object's memory |
| `split(str, delim)` | `split(str, delimiter)` | Splits string into a list of substrings |
| `join(list, delim)` | `join(list, delimiter)` | Joins list elements into a single string |
| `upper(str)` | `upper(str)` | Returns uppercase version of string |
| `lower(str)` | `lower(str)` | Returns lowercase version of string |
| `trim(str)` | `trim(str)` | Strips leading and trailing whitespace |
| `trimleft(str)` / `trimright(str)` | `trimleft(str)` / `trimright(str)` | Strips leading or trailing whitespace |
| `contains(str/list, target)` | `contains(str_or_list, target)` | Checks if substring or item is present |
| `startswith(str, prefix)` | `startswith(str, prefix)` | Checks if string starts with prefix |
| `endswith(str, suffix)` | `endswith(str, suffix)` | Checks if string ends with suffix |
| `replace(str, old, new)` | `replace(str, old_sub, new_sub)` | Replaces occurrences of substring |
| `find(str/list, target)` | `find(str_or_list, target)` | Returns 0-based index or -1 if not found |
| `count(str/list, target)` | `count(str_or_list, target)` | Counts non-overlapping occurrences |
| `reverse(str/list)` | `reverse(str_or_list)` | Returns reversed string or list |
| `chars(str)` | `chars(str)` | Returns list of characters as strings |
| `bytes(str)` | `bytes(str)` | Returns list of integer ASCII byte values |

---

## Classes & OOP

Skylang has a full class-based object-oriented system.

### Defining a Class

```skylang
class Dog {
    this.name
    this.breed
    this.age

    init {
        takes(name, breed, age)
        this.name = name
        this.breed = breed
        this.age = age
    }

    bark {
        print(this.name + " says: Woof!")
    }

    info {
        print(this.name + " | " + this.breed + " | Age: " + this.age)
    }
}
```

### Key Rules

| Concept | Syntax | Notes |
|---------|--------|-------|
| Field declaration | `this.fieldName` | Declared at the top of the class body |
| Constructor | `init { ... }` | Uses `takes(...)` to accept arguments |
| Method definition | `methodName { ... }` | **No `f` keyword** — `f` is only for standalone functions |
| Method calls | `instance.method(...)` | Called with parentheses |
| Privacy | All fields are **private** by default | Access through public methods (getters/setters) |

### Creating Instances

```skylang
my_dog := Dog("Rex", "Labrador", 3)
my_dog.bark()        // Rex says: Woof!
my_dog.info()        // Rex | Labrador | Age: 3
```

### Full Example — Bank Account

```skylang
class BankAccount {
    this.owner
    this.balance

    init {
        takes(owner, initial_deposit)
        this.owner = owner
        this.balance = initial_deposit
    }

    deposit {
        takes(amount)
        this.balance += amount
        return this.balance
    }

    withdraw {
        takes(amount)
        if amount > this.balance {
            print("Insufficient funds!")
            return this.balance
        }
        this.balance -= amount
        return this.balance
    }

    getBalance {
        return this.balance
    }

    getOwner {
        return this.owner
    }
}

acc := BankAccount("Aakash", 1000)
acc.deposit(500)
acc.withdraw(200)
print("Owner:", acc.getOwner())       // Aakash
print("Balance:", acc.getBalance())   // 1300
```

---

## Modules & Imports (Python-Style)

Skylang provides Python-style module importation for local `.sky` files and subdirectories, complete with `import`, `from ... import`, aliasing (`as`), and wildcard imports (`*`).

### 1. Standard Module Import

Import an entire module file by its name (relative to the current file or workspace):

```skylang
// examples/math_utils.sky
PI := 3.14159265
f square {
    takes(n)
    return n * n
}

// main.sky
import math_utils

println("PI:", math_utils.PI)
println("Square:", math_utils.square(8))
```

### 2. Import with Alias (`as`)

Use `as` to assign a local alias to an imported module or language bridge:

```skylang
import math_utils as mu
import python as py

println("PI:", mu.PI)
println("Square:", mu.square(10))

py_math := py.load("math")
println("Py sqrt:", py_math.sqrt(100))
```

### 3. Selective Symbol Import (`from ... import`)

Selectively import specific functions, classes, or variables directly into the local namespace:

```skylang
from math_utils import add, square, Calculator

println("add(10, 20):", add(10, 20))
println("square(6):", square(6))

calc := Calculator(100)
calc.add(50)
println("Total:", calc.get_val())
```

You can also alias individual imported symbols:

```skylang
from math_utils import square as sq, Calculator as Calc

println("sq(9):", sq(9))
c := Calc(25)
```

### 4. Wildcard Import (`from ... import *`)

Import all exported functions, classes, and global variables from a module directly into the caller's scope:

```skylang
from math_utils import *

println("PI:", PI)
println("Square of 12:", square(12))
```

### 5. Submodules & Nested Directories

Import modules located inside subdirectories using dotted notation:

```skylang
// Submodule at examples/sub/helper.sky
import sub.helper as sh
from sub.helper import greet

println(sh.format_info("Version", "1.0.0"))
println(greet("Developer"))
```

---

## Standard Library & Built-in Features

Skylang includes built-in standard library modules (`math`, `str`) alongside Python-style built-ins for I/O and string formatting, available out of the box without any import required:

### 1. `math` Module

```skylang
print(math.pi)                  // 3.14159...
print(math.e)                   // 2.71828...
print(math.sqrt(144))           // 12.0
print(math.abs(-42))            // 42
print(math.sin(0))              // 0.0
print(math.cos(0))              // 1.0
print(math.min(10, 20))         // 10
print(math.max(10, 20))         // 20
print(math.clamp(15, 0, 10))    // 10
print(math.hypot(3, 4))         // 5.0
```

### 2. Python-Style String Formatting & Built-in Helpers

Skylang uses Python-style string formatting and built-in number formatting functions rather than an external module:

```skylang
// String interpolation using .format()
msg := "Hello {}! Score: {}".format("Alice", 95)
print(msg)                     // Hello Alice! Score: 95

// Built-in number conversion functions
print(hex(255))                // ff
print(bin(42))                 // 101010
print(oct(64))                 // 100

// String methods & repetition
print("*-" * 5)                // *-*-*-*-*-
```

### 3. Python-Style File I/O & User Input

File operations and interactive terminal input follow clean Python-style built-ins (`open()`, `input()`):

```skylang
// Writing to a file
f := open("data.txt", "w")
f.write("Line 1\nLine 2\n")
f.close()

// Reading from a file
f = open("data.txt", "r")
content := f.read()
f.close()
print(content)

// Reading lines as a list
f = open("data.txt", "r")
lines := f.readlines()
f.close()
for line in lines {
    print(line)
}

// User input from stdin
name := input("Enter your name: ")
println("Hello,", name)
```

---

## Multi-Language Interoperability

Skylang provides first-class, bidirectional interoperability bridges with **Python**, **JavaScript/NPM**, **C++**, **Java**, **Golang**, and **Rust**. You can import ecosystem packages directly and call their methods and properties naturally using native Skylang syntax.

> **Single-Line Multi-Imports:** You can import multiple interoperability modules in a single statement separated by commas:
> ```skylang
> import python, js, cpp, java, go, rust
> ```
> *Note:* Skylang `import` statements strictly disallow dot notation (e.g. `import java.cpp` is invalid syntax and will raise a diagnostic error). Standard Go libraries like `fmt` must be loaded via Go's bridge `go.load("fmt")` rather than via `import fmt`.

### 1. Python Interoperability (`import python`)

Skylang embeds the Python 3 runtime C API directly with automatic bidirectional type marshalling:

```skylang
import python

// 1. Load Python standard library or third-party packages (e.g. numpy, math, sys)
math_py := python.load("math")
print("Python math.sqrt(256):", math_py.sqrt(256)) // 16.0
print("Python math.sin(0):", math_py.sin(0))       // 0.0

sys_py := python.load("sys")
print("Python sys.version_info:", sys_py.version_info)

// 2. Multi-library loading
mods := python.load("math", "sys")

// 3. Execute custom Python scripts/functions and invoke them
python.exec("
def greet(name):
    return f'Hello, {name} from Python 3!'
")
main_py := python.load("__main__")
msg := main_py.greet("Skylang")
print(msg)                                  // Hello, Skylang from Python 3!
```

### 2. JavaScript & NPM Interoperability (`import js`)

Skylang interfaces with Node.js and the NPM package ecosystem with bidirectional JSON marshalling. `js.load(...)` automatically detects whether the target is a JavaScript global (such as `Math` or `JSON`) or an NPM package (such as `lodash`, `axios`, or `express`):

```skylang
import js

// 1. Load JavaScript built-in globals
Math_js := js.load("Math")
print("JS Math.sqrt(625):", Math_js.sqrt(625))       // 25.0
print("JS Math.max(10, 99, 42):", Math_js.max(10, 99, 42)) // 99.0
print("JS Math.pow(2, 8):", Math_js.pow(2, 8))       // 256.0

// 2. Load NPM packages (auto-detected from node_modules)
// lodash := js.load("lodash")
// print(lodash.chunk([1, 2, 3, 4], 2))

// 3. Multi-package / global loading
mods := js.load("Math", "JSON")

// 4. Executing custom JavaScript code blocks
js.exec("console.log('Hello from Node.js runtime inside Skylang!');")
```

### 3. C++ Interoperability & JIT (`import cpp`)

Skylang features an on-the-fly C++ JIT compilation pipeline with dynamic linking:

```skylang
import cpp

// 1. Dynamic JIT compilation of C++ source code
cpp_math := cpp.compile("
extern \"C\" {
    double fast_power(double base, double exp) {
        return std::pow(base, exp);
    }
    double fast_poly(double x) {
        return 3.0 * x * x + 2.0 * x + 1.0;
    }
    double fast_hypot(double a, double b) {
        return std::hypot(a, b);
    }
}
")

print("C++ JIT fast_power(2.0, 10.0):", cpp_math.fast_power(2.0, 10.0)) // 1024.0
print("C++ JIT fast_poly(5.0):", cpp_math.fast_poly(5.0))               // 86.0
print("C++ JIT fast_hypot(30.0, 40.0):", cpp_math.fast_hypot(30.0, 40.0)) // 50.0

// 2. Load precompiled C++ shared libraries (.so)
// lib := cpp.load("./libmath.so")
```

### 4. Java Interoperability (`import java`)

Skylang interfaces with the Java Virtual Machine (OpenJDK) with full reflection and package loading:

```skylang
import java

// 1. Load standard or third-party Java classes
Math := java.load("java.lang.Math")
print("Java Math.sqrt(256.0):", Math.sqrt(256.0)) // 16.0
print("Java Math.max(42, 100):", Math.max(42, 100)) // 100
print("Java Math.PI:", Math.PI)             // 3.14159...

// 2. Multi-class loading
classes := java.load("java.lang.Math", "java.lang.String")

// 3. Executing Java code statements
java.exec("System.out.println(\"Java code execution works!\");")
```

### 5. Golang Interoperability (`import go` / `import golang`)

Skylang seamlessly interfaces with the Go runtime and supports on-the-fly `c-shared` binary compilation and standard package dispatch:

```skylang
import go

// 1. Load standard Go packages (including fmt, math, strings)
fmt := go.load("fmt")
fmt.Println("Hello from Go's fmt library via go.load()!")

math_go := go.load("math")
println("Go math.Sqrt(256.0):", math_go.Sqrt(256.0)) // 16.0
println("Go math.Pi:", math_go.Pi)          // 3.14159...

// 2. Multi-package loading
pkgs := go.load("math", "strings")

// 3. On-the-fly Go JIT compilation with exported functions
go_calc := go.compile("
//export Add
func Add(a, b float64) float64 {
    return a + b
}

//export FastPower
func FastPower(base, exp float64) float64 {
    res := 1.0
    for i := 0; i < int(exp); i++ {
        res *= base
    }
    return res
}
")

println("Go JIT Add(25.0, 17.0):", go_calc.Add(25.0, 17.0))          // 42.0
println("Go JIT FastPower(2.0, 8.0):", go_calc.FastPower(2.0, 8.0))  // 256.0

// 4. Executing Go statements
go.exec("fmt.Println(\"Hello from Go runtime!\")")
```

### 6. Rust Interoperability & JIT (`import rust`)

Skylang interfaces directly with `rustc` via `cdylib` dynamic linking with support for on-the-fly compiled `extern "C"` functions:

```skylang
import rust

// 1. Load Rust crates & standard constants
math_rs := rust.load("std::f64::consts")
println("Rust math PI:", math_rs.PI)        // 3.14159...
println("Rust math E:", math_rs.E)          // 2.71828...

// 2. Multi-module loading
mods := rust.load("std::f64::consts", "std::cmp")

// 3. On-the-fly Rust Dynamic JIT Compilation
rust_calc := rust.compile("
#[no_mangle]
pub extern \"C\" fn add(a: f64, b: f64) -> f64 {
    a + b
}

#[no_mangle]
pub extern \"C\" fn fast_power(base: f64, exp: f64) -> f64 {
    base.powf(exp)
}

#[no_mangle]
pub extern \"C\" fn fast_poly(x: f64) -> f64 {
    3.0 * x * x + 2.0 * x + 1.0
}
")

println("Rust JIT add(10.0, 32.0):", rust_calc.add(10.0, 32.0))             // 42.0
println("Rust JIT fast_power(2.0, 10.0):", rust_calc.fast_power(2.0, 10.0)) // 1024.0
println("Rust JIT fast_poly(5.0):", rust_calc.fast_poly(5.0))               // 86.0

// 4. Executing arbitrary Rust code blocks
rust.exec("println!(\"Hello from Rust runtime!\");")
```

---

## Foreign Function Interface (FFI)

Skylang can directly import C headers with `cimport` and declare `extern` C functions. Multiple header files can also be imported in a single statement:

```skylang
cimport "math.h", "stdio.h"

extern f cos(x)
extern f sin(x)
extern f sqrt(x)
extern f atan2(y, x)

print("C cos(0.0):", cos(0.0))       // 1.0
print("C sqrt(625):", sqrt(625.0))   // 25.0
```

---

## Dual Execution Architecture

Skylang features two complementary execution engines designed for development flexibility and production performance:

```
                               ┌──→ AST Codegen ──→ Clean C11 ──→ GCC -O2 ──→ Standalone Native Binary
 .sky Source ──→ Lexer ──→ Parser
                               └──→ AST Compiler ─→ Bytecode ───→ Stack VM ─→ Instant Execution & Profiler
```

### 1. Ahead-of-Time (AOT) C Transpiler
- **How it works**: Transpiles the high-level AST into optimized C11 code, and invokes GCC with `-O2`, linking against `libskylang_rt.a`, `libgc`, and multi-language interop shared objects.
- **When to use**: For maximum runtime speed, zero-overhead deployments, and distributing standalone binaries.
- **Commands**:
  ```bash
  sky run app.sky               # JIT compile & execute immediately
  sky build app.sky             # Produce standalone native binary `./app` silently
  sky build app.sky -o app_bin  # Produce standalone native binary with custom name
  ```

### 2. Stack-Based Bytecode Virtual Machine & Profiler
- **How it works**: Compiles the AST into compact 8-bit bytecode chunks containing opcodes (`OP_CONST`, `OP_CALL`, `OP_GET_LOCAL`, etc.) and executes on a lightweight virtual machine.
- **When to use**: For instant startup, scripting, debugging, and instruction-level performance inspection.
- **Commands**:
  ```bash
  sky vm app.sky                # Execute directly on the Bytecode VM
  sky profile app.sky           # Run with execution profiler enabled
  ```

#### Profiler Output Example
```text
======================= SKYLANG PERFORMANCE PROFILE =======================
Total Execution Time:   0.412 ms
Total Instructions:     1024
Bytecode Opcodes Executed:
  OP_GET_LOCAL    : 312
  OP_ADD          : 128
  OP_CALL         : 64
  ...
==========================================================================
```

---

## Error Handling & Traceback Engine

Skylang provides two complementary error paradigms: **Go-style explicit error values** for application domain logic, and a **Python-style Traceback Engine** that intercepts compilation and runtime failures with clean, human-readable diagnostics.

### 1. Go-Style Error Handling

Skylang supports explicit error return values — no bulky try/catch blocks required.

#### Returning Errors

A function signals an error by returning `error("message")`. A normal `return value` automatically sets the error to `none`:

```skylang
f safe_divide {
    takes(a, b)
    if b == 0 {
        return error("division by zero")
    }
    return a / b    // err is automatically none
}
```

#### Unpacking Results

Use tuple destructuring to capture both the result and the error:

```skylang
// Success case — err is automatically none
result, err := safe_divide(10, 2)
if err != none {
    print("Error:", err)
} else {
    print("Result:", result)    // Result: 5
}

// Error case — result is none, err has the message
res, err2 := safe_divide(10, 0)
if err2 != none {
    print("Caught:", err2)      // Caught: division by zero
}
```

#### The Blank Identifier `_`

Use `_` to discard a value you don't need:

```skylang
ans, _ := safe_divide(100, 4)     // ignore the error
print("Answer:", ans)              // 25

_, err_only := safe_divide(10, 0) // ignore the result
print("Error was:", err_only)
```

#### Multi-Variable Assignment & Swapping

```skylang
a, b := 10, 20
a, b = b, a        // swap: a=20, b=10
```

#### Panic — Unrecoverable Errors

For situations where the program **cannot safely continue**, use `panic()`:

```skylang
if critical_failure {
    panic("Fatal: unable to initialize memory buffer")
}
```

`panic()` immediately halts the program and displays the fatal diagnostic.

---

### 2. Python-Style Traceback Engine

Skylang guarantees that developers are **never exposed to raw C compiler errors, mangled C symbols, or internal code generator output**. All parse errors, compilation failures, and runtime exceptions are processed by Skylang's diagnostic engine to print Python-style tracebacks with exact source files, line numbers, line excerpts, and call frames.

#### Syntax & Parse Errors
When syntax is invalid, Skylang prints a formatted `SyntaxError` with the exact file location, code snippet, and visual caret (`^`) pointing to the offending column:

```text
  File "main.sky", line 5
    x := 10 +
            ^
SyntaxError: Unexpected token
```

#### Compile-Time Diagnostics
C compiler errors are automatically intercepted and translated into clean Skylang diagnostics:

```text
  File "app.sky", line 14
    unknown_variable = 42
    ^
NameError: 'unknown_variable' undeclared
```

#### Runtime Stack Tracebacks
When a runtime fault or unhandled error occurs, Skylang unwinds the execution call stack and displays the full trace:

```text
Traceback (most recent call last):
  File "server.sky", line 45, in handle_request
    data := process_payload(raw)
  File "server.sky", line 28, in process_payload
    ratio := 100 / count
ZeroDivisionError: division by zero
```

#### Standard Exception Hierarchy

| Exception | Condition |
|---|---|
| `SyntaxError` | Invalid syntax or unexpected token during parsing |
| `NameError` | Accessing an undeclared identifier or symbol |
| `TypeError` | Unsupported operand types or invalid argument counts |
| `ZeroDivisionError` | Division or modulo by zero (`/`, `//`, `%`) |
| `IndexError` | List index, array bound, or string slice out of range |
| `AttributeError` | Accessing a non-existent property or method on an object |
| `ModuleNotFoundError`| Attempting to load an uninstalled or missing native/foreign module |
| `FileNotFoundError` | I/O operation on a file path that does not exist |
| `PermissionError` | File system or system call permission denied |

---

## Memory Management

### Automatic Garbage Collection

Skylang uses the **Boehm-Demers-Weiser GC** (`libgc`) — the same approach used by many production systems. You don't need to manually free objects, strings, or collections. When an object is no longer referenced, the runtime automatically reclaims its memory.

### Manual Controls

For advanced use cases, two manual tools are available:

```skylang
gc()           // Force an immediate garbage collection cycle
free(my_obj)   // Manually deallocate a specific object
```

---

## Comments

```skylang
// This is a single-line comment

/* This is
   a multi-line
   comment */
```

---

## VS Code Extension & Editor Support

Skylang includes a full-featured, rich Visual Studio Code extension designed for a seamless developer experience:

- **One-Click Execution**: Dedicated `$(play) Run` button in the editor title bar and inline CodeLens for instant single-click execution.
- **IntelliSense & Smart Snippets**:
  - Auto-completion with intelligent argument placeholders for `.load("")`, `.exec("")`, and `.compile("")` with cursor placed inside the quotes.
  - Global `eval("...")` built-in code completion and evaluation snippets.
  - Dynamic module suggestions for unimported packages (e.g. typing `go.`, `rust.`, `python.` automatically suggests imports).
  - Snippets for functions (`f`), variadic functions (`fvar`), classes (`class`), loops (`forin`, `forcond`), error handling (`errhandle`), and multi-language interop (`cimport`, `imppy`, `impjs`, `impcpp`, `impjava`, `goload`, `rustload`).
- **Real-Time Diagnostics & Linter**: Direct inline squiggly error diagnostics mapped directly to `.sky` lines in the editor.
- **Rich Syntax Highlighting**: Comprehensive TextMate grammar covering keywords, scalar and collection types, bracketless functions, `takes(...)`, multi-module single-line imports (`import python, js, cpp, java, go, rust`), and FFI declarations.
- **Workspace Build Tasks**: Run (`sky run`), build (`sky build`), or VM test directly inside VS Code via `Ctrl+Shift+B` or Command Palette (`Tasks: Run Task`).

### Installing the VS Code Extension

The extension is installed automatically when running `./install.sh`. You can also install or update it at any time with:

```bash
make vscode
```

---

## Project Structure

```
skylang/
├── install.sh            # One-command install & environment setup script
├── delete.sh             # Uninstaller script
├── Makefile              # Build system (includes `make vscode`)
├── README.md              # Complete guide & documentation
├── .vscode/               # Workspace settings & build tasks
├── editors/
│   └── vscode/            # VS Code extension (grammar, snippets, manifest)
├── bin/                   # Compiled binaries (skylang, sky)
├── examples/              # Example programs
│   ├── 01_basics.sky      #   Variables, operators, walrus, .T
│   ├── 02_collections.sky #   Strings, arrays, lists, tuples, dicts, sets
│   ├── 03_control_flow.sky#   If/elif/else, for loops
│   ├── 04_functions.sky   #   Functions, takes(), recursion
│   ├── 05_classes.sky     #   Classes, init, methods, instances
│   ├── 06_error_handling.sky # Error handling, multiple returns, blank _
│   ├── 07_stdlib.sky      #   Standard library (math, str, Python-style I/O & formatting)
│   ├── 08_ffi.sky         #   C header cimport and extern declarations
│   ├── 09_vm_profile.sky  #   Bytecode VM execution and profiling
│   ├── 10_python.sky      #   Python interop and package loading
│   ├── 11_js.sky          #   JavaScript & NPM package interop
│   ├── 12_cpp.sky         #   C++ interop and dynamic JIT compilation
│   ├── 13_java.sky        #   Java interop and reflection
│   ├── 14_go.sky          #   Golang interop and c-shared JIT compilation
│   ├── 15_rust.sky        #   Rust interop and cdylib JIT compilation
│   ├── 16_modules.sky     #   Python-style imports (import, from, as, *)
│   ├── math_utils.sky     #   Example math helper module
│   └── sub/
│       └── helper.sky     #   Example nested submodule
├── include/               # Header files
│   ├── skylang.h          #   Compiler internals (lexer, parser, codegen)
│   ├── skylang_rt.h       #   Runtime API (Value, GC, collections, OBJ_FOREIGN)
│   ├── sky_stdlib.h       #   Standard library API (math, str, built-in I/O)
│   ├── sky_vm.h           #   Bytecode VM, chunk format, profiler
│   ├── sky_python.h       #   Python C API bridge
│   ├── sky_js.h           #   Node.js / NPM interop bridge
│   ├── sky_cpp.h          #   C++ JIT compilation bridge
│   ├── sky_java.h         #   Java reflection interop bridge
│   ├── sky_go.h           #   Golang interop bridge
│   └── sky_rust.h         #   Rust interop bridge
├── lib/                   # Static runtime library (libskylang_rt.a)
├── tests/                 # Test suite
│   ├── test_all.sky       #   Comprehensive automated test suite
│   └── test_eval.sky      #   Dynamic eval() test suite
└── src/                   # Source code
    ├── main.c             #   CLI driver (run, build)
    ├── lexer.c            #   Tokenizer
    ├── parser.c           #   Recursive descent parser → AST
    ├── codegen.c          #   AST → C code transpiler
    ├── compiler.c         #   AST → Bytecode compiler
    ├── vm.c               #   Stack-based VM interpreter & dynamic eval engine
    ├── runtime.c          #   Runtime: Value system, GC, collections, dynamic dispatch, traceback engine
    ├── sky_stdlib.c       #   Standard library implementation
    ├── sky_python.c       #   Python interop bridge
    ├── sky_js.c           #   Node.js / NPM interop bridge
    ├── sky_cpp.c          #   C++ JIT bridge
    ├── sky_java.c         #   Java interop bridge
    ├── sky_go.c           #   Golang interop bridge
    └── sky_rust.c         #   Rust interop bridge
```

### How Compilation Works

```
 .sky file → Lexer → Tokens → Parser → AST ──┬──→ Codegen ──→ C code ──→ GCC ──→ Native Binary
                                            └──→ Compiler ─→ Bytecode ────────→ Bytecode VM (eval)
```

---

## Running Tests

```bash
make test
```

The test suite (`tests/test_all.sky`) covers **24 categories** — all passing:

1. Default initializations (`I=0`, `D=0.0`, `B=false`, `C='a'`, `S=""`)
2. Walrus operator `:=`
3. Arithmetic operators (`+`, `-`, `*`, `/`, `//`, `^`, `%`)
4. Comparisons and boolean logic (`and`, `or`, `!`)
5. String indexing, slicing, built-in methods, and properties (`.size`, `.chars`, `.bytes`)
6. Fixed-size arrays (`<val1, val2>` and `arr I 5`)
7. Dynamic lists (`push`, `pop`, `clear`, slicing)
8. Tuples
9. Dictionaries (`has`, `size`, property access)
10. Sets (`add`, `has`, deduplication)
11. Sorted lists (auto-sorted insertion)
12. Type system and `.T` property
13. Functions, `takes(...)`, and recursion
14. Control flow (`if`, `elif`, `else`, unified `for` loops)
15. Classes, constructors, fields, and methods
16. Standard Library modules & Built-ins (`math`, `str`, Python-style I/O & formatting)
17. Foreign Function Interface (`cimport`, `extern f`)
18. Go-style error handling, multiple returns, and blank `_`
19. Python Interoperability (`import python`, embedded C API)
20. JavaScript / NPM Interoperability (`import js`)
21. C++ Interoperability & Dynamic JIT (`import cpp`)
22. Java Interoperability (`import java`, reflection bridge)
23. Golang Interoperability & c-shared JIT (`import go`, `import golang`)
24. Rust Interoperability & cdylib JIT (`import rust`)

---

## Roadmap

- [x] **FFI (Foreign Function Interface)** — Direct C header `cimport` and `extern f` function bindings
- [x] **Standard Library & Built-ins** — `math`, `str`, and Python-style I/O & formatting built-in
- [x] **Built-in String Operations** — Full suite of built-in string methods, properties, and functions
- [x] **Dynamic eval(...) Engine** — Built-in dynamic expression and statement evaluation
- [x] **Multi-Language Interoperability** — Native language bridges for Python, JavaScript/NPM, C++, Java, Golang, and Rust
- [x] **Python-Style Traceback Engine** — Clean compile-time and runtime error tracebacks with call stack unwinding
- [ ] **Package Manager** — Import and share Skylang packages

---

## License

This project is currently unlicensed. All rights reserved.
