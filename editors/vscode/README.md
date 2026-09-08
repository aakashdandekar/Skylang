<p align="center">
  <img src="icon.png" alt="Skylang Logo" width="180" style="border-radius: 14px;" />
</p>

# Skylang for Visual Studio Code

[![VS Code Extension](https://img.shields.io/badge/VS%20Code-v1.80%2B-blue.svg?logo=visual-studio-code)](https://marketplace.visualstudio.com)
[![Language](https://img.shields.io/badge/language-Skylang-orange.svg)]()
[![Version](https://img.shields.io/badge/version-v1.0.0-brightgreen.svg)]()
[![Target](https://img.shields.io/badge/target-C11%20%7C%20GCC%20%7C%20Bytecode%20VM-blue.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Comprehensive, first-class language support for the **Skylang** programming language in Visual Studio Code. This extension provides advanced **IntelliSense**, **Snippets**, **Context-Aware Autocomplete**, **Hover Documentation**, **Signature Help**, **Go-to-Definition**, **Document Outline**, **Automatic Code Formatting**, **Real-time Diagnostics & Linting**, **One-Click QuickFix Actions**, and **Integrated Execution & Profiling** via Skylang's AOT and Bytecode VM engines.

---

## 📑 Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
  - [1. 🌈 Syntax Highlighting](#1--syntax-highlighting)
  - [2. 💡 Context-Aware IntelliSense & Member Autocomplete](#2--context-aware-intellisense--member-autocomplete)
  - [3. 📖 Rich Hover Documentation](#3--rich-hover-documentation)
  - [4. 🔤 Interactive Signature Help & Parameter Hints](#4--interactive-signature-help--parameter-hints)
  - [5. ⚡ Comprehensive Snippets Library](#5--comprehensive-snippets-library)
  - [6. 🔍 Outline View & Go-to-Definition](#6--outline-view--go-to-definition)
  - [7. 🧹 Code Formatting](#7--code-formatting)
  - [8. 🚨 Real-time Diagnostics, Linter & QuickFixes](#8--real-time-diagnostics-linter--quickfixes)
  - [9. ▶ CodeLens & Integrated Execution (AOT & VM)](#9--codelens--integrated-execution-aot--vm)
- [Skylang Language Quick Reference](#skylang-language-quick-reference)
- [Snippets Cheat Sheet](#snippets-cheat-sheet)
- [Extension Commands](#extension-commands)
- [Extension Settings](#extension-settings)
- [Installation & Setup](#installation--setup)
- [Building from Source](#building-from-source)
- [License](#license)

---

## Overview

**Skylang** is a high-performance compiled, dynamic, object-oriented language built upon C. It unites:
- **Python's** dynamic ergonomics, clean syntax, Python-style I/O & formatting, and list/string slicing
- **Go's** unified `for` loop syntax and explicit error handling
- **C's** native execution speed via C11 ahead-of-time transpilation and direct C header `cimport`
- **Multi-Language Interoperability** with native bridges for **Python**, **JavaScript/NPM**, **C++**, **Java**, **Golang**, and **Rust**

This extension brings complete IDE superpowers to your Skylang development workflow.

---

## Key Features

### 1. 🌈 Syntax Highlighting

Full TextMate grammar support for `.sky` and `.skylang` files:
- **Keywords**: `f`, `class`, `init`, `this`, `takes`, `return`, `if`, `else`, `elif`, `for`, `in`, `break`, `continue`, `cimport`, `extern`, `import`, `new`
- **Scalar Types**: `I` (int), `D` (double), `B` (bool), `C` (char), `S` (string)
- **Collection Types**: `L` (list), `T` (tuple), `SL` (sortedList), `DICT` (dict), `SET` (set), and fixed arrays (`arr I 5`, `<1, 2, 3>`)
- **Operators**: Walrus `:=`, compound assignments (`+=`, `-=`, `*=`, `/=`), arithmetic (`+`, `-`, `*`, `/`, `//`, `^`, `%`), relational (`==`, `!=`, `<`, `<=`, `>`, `>=`), and logical (`and`, `or`, `!`, `&&`, `||`)
- **Built-in Functions**: `print`, `println`, `open`, `input`, `hex`, `bin`, `oct`, `format`, `range`, `len`, `type`, `takes`, `error`, `panic`, `gc`, `free`, `eval`
- **Properties**: `.T` (type descriptor) and `.size` (O(1) container size)
- **Built-in Modules & Interop**: `math`, `str`, `python`, `js`, `cpp`, `java`, `go`, `golang`, `rust`

```skylang
// Example: Multi-Language Interop & Python-Style I/O in Skylang
import python, js, go

// File I/O (Python syntax)
f := open("sample.txt", "w")
f.write("Hello from Skylang!\n")
f.close()

// String Formatting (Python syntax)
msg := "Language: {}, Base: {}".format("Skylang", "C11")
println(msg)

// Loading Go's fmt library via go.load()
fmt := go.load("fmt")
fmt.Println("Loaded via Go bridge!")
```

---

### 2. 💡 Context-Aware IntelliSense & Member Autocomplete

IntelliSense adapts dynamically to the current editing context:
- **Built-in & Interop Module Trigger (`.`)**:
  - `math.` → `sqrt`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `abs`, `ceil`, `floor`, `round`, `log`, `log10`, `log2`, `exp`, `pow`, `min`, `max`, `hypot`, `clamp`, `pi`, `e`, `inf`, `nan`
  - `str.` → `split`, `join`, `upper`, `lower`, `trim`, `trimleft`, `trimright`, `contains`, `startswith`, `endswith`, `replace`, `find`, `count`, `reverse`, `chars`, `bytes`, `format`, `rjust`, `ljust`, `center`, `zfill`
  - `python.` → `load`, `exec`, `eval`
  - `js.` → `load`, `exec`, `eval`
  - `cpp.` → `compile`, `load`, `eval`
  - `java.` → `load`, `exec`, `eval`
  - `go.` → `load`, `compile`, `exec`
  - `rust.` → `load`, `compile`, `exec`
- **File Object Autocompletion (`open()`)**:
  - `f.` (where `f := open(...)`) → `.read()`, `.write()`, `.readline()`, `.readlines()`, `.close()`, `.size`, `.T`
- **Type-Aware Member Autocompletion**:
  - `tasks.` (where `tasks L` or `[1, 2]`) → `.push()`, `.pop()`, `.clear()`, `.size`, `.T`
  - `scores.` (where `scores DICT`) → `.has()`, `.size`, `.T`
  - `tags.` (where `tags SET`) → `.add()`, `.has()`, `.size`, `.T`
  - `grades.` (where `grades SL`) → `.push()`, `.pop()`, `.size`, `.T`
  - `name.` (where `S name` or `"text"`) → `.format()`, `.upper()`, `.lower()`, `.trim()`, `.size`, `.T`
  - `my_obj.` (where `my_obj := ClassName(...)`) → `.method()`, `.field`, `.T`
- **Class Context (`this.`)**: Suggests instance fields and methods declared in the active class.
- **Strict `import` Autocomplete**:
  - `import ` → `python`, `js`, `cpp`, `java`, `go`, `golang`, `rust` *(Skylang strictly disallows dot-notation imports and flags standard libraries; Go's `fmt` is loaded via `go.load("fmt")`)*.
- **C FFI Autocomplete**:
  - `cimport "` → `"math.h"`, `"stdio.h"`, `"stdlib.h"`, `"string.h"`, `"time.h"`, `"unistd.h"`
  - `extern f ` → `cos(x)`, `sin(x)`, `tan(x)`, `sqrt(x)`, `atan2(y, x)`, `pow(base, exp)`, `strlen(str)`, `strcmp(s1, s2)`
- **Foreign Package Suggestions**:
  - `python.load("` → `numpy`, `pandas`, `scipy`, `math`, `sys`, `os`, `json`, `random`, `requests`
  - `go.load("` → `fmt`, `math`, `strings`, `time`, `net/http`, `os`
  - `java.load("` → `java.lang.Math`, `java.lang.String`, `java.util.ArrayList`, `java.util.HashMap`
- **Named Arguments**: Suggests `param=` inside function, method, and constructor calls matching the target's `takes(...)` signature.
- **Dynamic Symbol Indexing**: Autocompletes user-defined functions, classes, methods, and variables declared in your file and workspace.

---

### 3. 📖 Rich Hover Documentation

Hover over any language symbol to see its full signature, documentation, parameters, return types, and runnable code examples:

| Hover Target | Information Displayed |
|---|---|
| **Keywords** (`f`, `takes`, `class`, `import`, `cimport`, `for`, etc.) | Detailed explanation, syntax grammar, and code example |
| **Types** (`I`, `D`, `B`, `C`, `S`, `L`, `T`, `DICT`, `SET`, `SL`) | Bit-width, default values, memory properties, and sample declarations |
| **Built-in Functions** (`open`, `input`, `hex`, `bin`, `oct`, `format`, `print`, `println`, `range`, `takes`, `error`, etc.) | Signature, parameter descriptions, return value, and Python-style usage examples |
| **Stdlib Methods** (`math.sqrt`, `str.split`, `str.format`, etc.) | Full parameter list, return type, and usage example |
| **Interop Modules** (`python.load`, `js.load`, `cpp.compile`, `java.load`, `go.load`, `rust.load`) | Multi-language bridge documentation and examples |
| **C FFI & Headers** (`cimport "math.h"`, `extern f cos(x)`) | C header documentation and native function signatures |
| **User Symbols** | Function signatures with parameters, class declarations, fields, and typed variables |

---

### 4. 🔤 Interactive Signature Help & Parameter Hints

Typing `(` or `,` automatically triggers interactive parameter hints:
- Shows the full function signature
- Highlights the **active parameter** you are currently editing (including named arguments `param=`)
- Works for built-in functions (`open`, `input`, `hex`, `bin`, `oct`, `format`, `print`, `range`), stdlib modules, interop bridges, collection methods, extern C functions, and user-defined functions/constructors

---

### 5. ⚡ Comprehensive Snippets Library

Over **75 productivity snippets** designed for rapid coding. Simply type the prefix and press `Tab` or `Enter`.

#### Variable & Type Declarations
| Prefix | Expansion | Description |
|---|---|---|
| `decl` | `I var = 0` / `S var = ""` | Initialize scalar variable (`I`, `D`, `B`, `C`, `S`) |
| `:=` | `name := value` | Declare variable with type inference |
| `list` | `items := [1, 2, 3]` | Dynamic list literal |
| `dict` | `data := {"key": value}` | Dictionary hash map |
| `set` | `tags SET` | Set of unique elements |

#### Functions & Classes
| Prefix | Expansion | Description |
|---|---|---|
| `f` / `fn` | `f name { takes(a, b) ... }` | Bracketless function with `takes(...)` |
| `fvar` | `f name { for i in args { ... } }` | Variadic function using `args` list |
| `takes` | `takes(a, b)` | Parameter filter gateway |
| `ret` | `return val` | Return statement |
| `class` | `class Name { this.field \n init { takes(...) } ... }` | Object-Oriented class template |
| `init` | `init { takes(...) this.field = field }` | Class constructor block |
| `method` | `method_name { takes(...) ... }` | Class method (without `f` keyword) |
| `err` | `res, err := fn() \n if err != none { ... }` | Go-style error check and unpack |

#### Control Flow & Loops
| Prefix | Expansion | Description |
|---|---|---|
| `if` | `if condition { ... }` | If statement |
| `ifelse` | `if cond { ... } else { ... }` | If-else statement |
| `ifelif` | `if c1 { ... } elif c2 { ... } else { ... }` | If-elif-else statement |
| `forin` | `for item in collection { ... }` | Iterator loop |
| `forcond` | `for condition { ... }` | Condition loop (while equivalent) |
| `forinf` | `for { if cond { break } ... }` | Infinite loop |
| `forr` | `for i in range(10) { ... }` | Numeric range loop |
| `println` | `println(...)` | Print with newline |
| `print` | `print(...)` | Print without newline |
| `eval` | `res := eval("...")` | Dynamic Skylang expression eval |

#### Python-Style I/O & Formatting
| Prefix | Expansion | Description |
|---|---|---|
| `open` | `f := open("data.txt", "r") \n content := f.read() \n f.close()` | Open and read file (Python-style) |
| `openw` | `f := open("data.txt", "w") \n f.write(content) \n f.close()` | Open and write to file (Python-style) |
| `openlines` | `f := open(...) \n lines := f.readlines() \n f.close()` | Read all lines as a list (Python-style) |
| `input` | `val := input("Prompt: ")` | Read stdin input |
| `format` | `"Hello, {}!".format(name)` | Python-style `{}` string formatting |
| `hex` | `hex(number)` | Integer to hexadecimal string |
| `bin` | `bin(number)` | Integer to binary string |
| `oct` | `oct(number)` | Integer to octal string |

#### Multi-Language Interoperability & FFI
| Prefix | Expansion | Description |
|---|---|---|
| `import` | `import python` / `import go` | Import multi-language bridge |
| `cimport` | `cimport "math.h"` | C header import for native FFI |
| `extern` | `extern f name(params)` | C function declaration |
| `pyload` | `mod := python.load("numpy")` | Load Python module |
| `pyexec` | `python.exec("...")` | Execute Python script block |
| `jsload` | `mod := js.load("lodash")` | Load Node.js / NPM package |
| `jsexec` | `js.exec("...")` | Execute JavaScript code |
| `cppcompile` | `mod := cpp.compile("...")` | JIT-compile C++ source |
| `cppload` | `lib := cpp.load("./libnative.so")` | Load native shared library |
| `javaload` | `cls := java.load("java.lang.Math")` | Load Java class via JVM reflection |
| `javaexec` | `java.exec("...")` | Execute Java statements |
| `goload` | `mod := go.load("fmt")` / `go.load("./lib.so")` | Load Go package or shared library |
| `gocompile` | `mod := go.compile("...")` | JIT-compile Go code |
| `goexec` | `go.exec("...")` | Execute Go code |
| `rustload` | `mod := rust.load("./librust.so")` | Load Rust cdylib |
| `rustcompile` | `mod := rust.compile("...")` | JIT-compile Rust code |
| `rustexec` | `rust.exec("...")` | Execute Rust code |

---

### 6. 🔍 Outline View & Go-to-Definition

- **Document Outline & Breadcrumbs**: Displays class hierarchies, methods, fields, and functions in the VS Code Explorer Outline panel.
- **Go to Definition (`F12` / `Ctrl+Click`)**: Instantly jump from calls to definitions for functions, classes, methods, fields, and variables.
- **Workspace Symbol Search (`Ctrl+T` / `Cmd+T`)**: Quick-search any symbol across all `.sky` files in your workspace.

---

### 7. 🧹 Code Formatting

Automatic document formatting via `Shift+Alt+F` (Windows/Linux) or `Shift+Option+F` (macOS):
- Standardizes indentation for nested blocks and classes
- Cleans up spacing around operators (`:=`, `=`, `+`, `-`, `*`, `/`, `//`, `^`, `%`, `==`, `!=`, `<`, `>`)
- Formats keyword headers (`if`, `elif`, `for`, `class`, `f`, `takes`)
- Cleans up comma spacing and removes trailing whitespace

---

### 8. 🚨 Real-time Diagnostics, Linter & QuickFixes

Provides instant feedback as you write code:
- **Strict `import` Syntax Validation (`skylang-invalid-import-syntax`)**:
  - Skylang `import` statements only accept comma-separated module names (e.g. `import python, js, cpp, java, go, rust`).
  - Dot notation like `import java.cpp` is flagged with an automated QuickFix code action to convert it to `import java, cpp`.
- **Foreign Bridge Enforcement (`skylang-invalid-import-module`)**:
  - `import fmt` is flagged: Go's `fmt` library must be loaded via `go.load("fmt")`. An automatic QuickFix is provided to generate `import go\nfmt := go.load("fmt")`.
  - `import io` is flagged: Skylang uses Python-style built-in `open()`, `input()`, `f.read()`, etc.
- **Python-Style I/O & Formatting Enforcement (`skylang-invalid-module-usage`)**:
  - Detects deprecated module calls like `io.readfile()` or `fmt.format()` and provides clear migration reminders to `open()`, `input()`, `"{}.format()"`, `hex()`, `bin()`, `oct()`.
- **Bracketless Function Check (`skylang-bracketless-function`)**:
  - Warns if a function is declared with parentheses (e.g. `f add(a, b)`) and suggests using Skylang's `takes(...)` gateway syntax.
- **Syntax Integrity**: Flags unclosed braces `{ }`, parentheses `( )`, brackets `[ ]`, and unterminated strings.

---

### 9. ▶ CodeLens & Integrated Execution (AOT & VM)

Run and profile Skylang files directly from VS Code:

- **Top-of-File CodeLens Buttons**:
  - `$(play) Run (AOT)`: Transpile to C and execute immediately with GCC (`sky run file.sky`)
  - `$(zap) Run VM`: Execute on the Skylang Bytecode Virtual Machine (`sky vm file.sky`)
  - `$(dashboard) Profile VM`: Execute with opcode performance profiler (`sky profile file.sky`)
- **Right-Click Context Menu & Editor Title Actions**: Quick actions to Run, Run in VM, Profile, or Build Native Binary.
- **Integrated Terminal**: Output is displayed cleanly in a dedicated `Skylang` VS Code terminal.

---

## Skylang Language Quick Reference

```skylang
// 1. Scalar Types & Defaults
I count = 10         // int (default 0)
D pi = 3.14159       // double (default 0.0)
B active = true      // bool (default false)
C initial = 'A'      // char (default 'a')
S title = "Skylang"  // string (default "")

// 2. Walrus Type Inference
score := 95

// 3. Collections
primes I = <2, 3, 5, 7>          // Fixed array
tasks L                          // Dynamic list
tasks.push("Write code")
coords T = (10, 20, 30)          // Immutable tuple
scores DICT                      // Hash map
scores["Alice"] = 95
tags SET                         // Unique set
tags.add("skylang")
grades SL                        // Auto-sorted list
grades.push(85)

// 4. Bracketless Functions & takes(...) Gateway
f add {
    takes(a, b)
    return a + b
}

// 5. Classes & Methods (fields are private with this., methods omit f)
class Dog {
    this.name
    this.breed

    init {
        takes(name, breed)
        this.name = name
        this.breed = breed
    }

    bark {
        println(this.name, "woofs!")
    }
}

// 6. Python-Style File I/O & String Formatting
f := open("output.txt", "w")
f.write("Score: {}\n".format(score))
f.close()

println("Formatted:", "Hex: {}, Bin: {}".format(hex(255), bin(42)))

// 7. Go-Style Error Handling
f safe_divide {
    takes(a, b)
    if b == 0 {
        return error("division by zero")
    }
    return a / b
}

res, err := safe_divide(10, 2)
if err != none {
    println("Error:", err)
} else {
    println("Result:", res)
}

// 8. Multi-Language Interop
import python, js, go

math_py := python.load("math")
println("Python sqrt(256):", math_py.sqrt(256))

fmt := go.load("fmt")
fmt.Println("Hello from Go runtime!")
```

---

## Extension Commands

| Command | Title | Keybinding / Context |
|---|---|---|
| `skylang.run` | **Skylang: Run File (AOT)** | Editor Title Play Button, Context Menu, CodeLens |
| `skylang.runVM` | **Skylang: Run File in Bytecode VM** | Editor Title Zap Button, Context Menu, CodeLens |
| `skylang.profile` | **Skylang: Profile in Bytecode VM** | Editor Title Dashboard Button, Context Menu, CodeLens |
| `skylang.build` | **Skylang: Build Native Binary** | Editor Context Menu, Command Palette |
| `skylang.repl` | **Skylang: Start Interactive REPL** | Command Palette |
| `skylang.formatDocument` | **Skylang: Format Document** | `Shift+Alt+F` / `Cmd+Shift+I` |

---

## Extension Settings

Customize extension behavior in your VS Code `settings.json`:

```json
{
    // Path to the `sky` compiler executable (searches PATH / project bin by default)
    "skylang.executablePath": "/home/aakashdandekar/Projects/programming_lang/bin/sky",

    // Enable real-time syntax and structural diagnostics
    "skylang.diagnostics.enabled": true,

    // Indentation size in spaces for Skylang formatter
    "skylang.format.indentSize": 4
}
```

---

## Installation & Setup

### Automatic Installation

The extension is automatically installed in your local VS Code environment under:
```bash
~/.vscode/extensions/aakashdandekar.skylang-snippets-intellisense-1.0.0/
```

### Compiler Setup
Ensure the Skylang compiler is built and available:
```bash
cd ~/Projects/programming_lang
make clean && make
```
The extension will automatically detect the binary at `~/Projects/programming_lang/bin/sky` or globally via `~/.local/bin/sky`.

---

## Building from Source

To modify or rebuild the extension:

```bash
cd ~/Projects/snippets_skylang
npm install
npm run compile
```

To watch for changes during development:
```bash
npm run watch
```

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.
