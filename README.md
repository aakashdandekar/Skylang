<p align="center">
  <img src="assets/skylang_logo.jpg" alt="Skylang Logo" width="220" style="border-radius: 16px;" />
</p>

<h1 align="center">Skylang Programming Language</h1>

<p align="center">
  <strong>A modern, friendly, and high-performance programming language designed for everyone — from complete beginners to systems engineers.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg" alt="License: Apache 2.0" />
  <img src="https://img.shields.io/badge/build%20%26%20test-passing%20(24%2F24)-brightgreen.svg" alt="Build & Test" />
  <img src="https://img.shields.io/badge/target-C11%20%7C%20GCC%20--O2-blue.svg" alt="C11 & GCC" />
  <img src="https://img.shields.io/badge/GC-Boehm--Demers--Weiser-orange.svg" alt="Memory Management" />
  <img src="https://img.shields.io/badge/interop-Python%20%7C%20JS%20%7C%20C%2B%2B%20%7C%20Java%20%7C%20Go%20%7C%20Rust-purple.svg" alt="Multi-Language" />
</p>

---

## 🌟 What is Skylang? (A Quick Introduction)

If you have never written a line of code in your life, think of a **programming language** as a way to give clear, step-by-step instructions to your computer. 

Usually, programming languages fall into two extremes:
1. **Easy to learn, but slow**: Languages like Python are wonderful to read and write, but they can be slower when crunching big data or doing heavy calculations.
2. **Extremely fast, but hard to learn**: Languages like C or C++ run at blistering speeds, but they require dealing with complex memory management, pointers, and verbose syntax.

**Skylang brings the best of both worlds together**:
- **Reads like plain English**: Write clean, expressive code without confusing boilerplate or messy brackets.
- **Runs with native C speed**: Behind the scenes, Skylang converts your code into optimized **C11** and compiles it into a blazing-fast standalone executable.
- **Zero memory worries**: Automatic Garbage Collection takes care of cleaning up memory for you.
- **Universal Multi-Language Bridge**: Want to use a Python library, an NPM JavaScript package, a Java class, a Go module, or high-speed Rust code? Skylang can call them all natively in the exact same program!

---

## 📖 First Look: Your Very First Skylang Program

Let's look at a simple Skylang program:

```skylang
// 1. Storing information in a variable
name := "Alex"
age := 20

// 2. Printing a greeting using an f-string (formatted text)
println(f"Hello, {name}! In 5 years, you will be {age + 5} years old.")

// 3. Making decisions
if age >= 18 {
    println("Status: You are an adult!")
} else {
    println("Status: You are a minor.")
}

// 4. Storing a list of favorite hobbies
hobbies := ["Coding", "Music", "Gaming"]
println(f"You have {hobbies.size} hobbies:")

// 5. Repeating an action with a loop
for hobby in hobbies {
    println(f"- I love {hobby}")
}
```

**Output when you run this:**
```text
Hello, Alex! In 5 years, you will be 25 years old.
Status: You are an adult!
You have 3 hobbies:
- I love Coding
- I love Music
- I love Gaming
```

---

## 📑 Table of Contents

1. [Installation & Setup](#1-installation--setup)
2. [Running Your First Program (CLI Usage)](#2-running-your-first-program-cli-usage)
3. [Variables & Data Types (Storing Information)](#3-variables--data-types-storing-information)
   - [What is a Variable?](#what-is-a-variable)
   - [The Walrus Operator `:=` (Automatic Type Inference)](#the-walrus-operator---automatic-type-inference)
   - [Explicit Type Prefixes (`I`, `D`, `B`, `C`, `S`)](#explicit-type-prefixes-i-d-b-c-s)
   - [Default Values](#default-values-automatic-initialization)
   - [Checking Types with `.T`](#checking-types-with-t)
4. [Operators (Doing Math & Logic)](#4-operators-doing-math--logic)
   - [Arithmetic Operators (Math)](#arithmetic-operators-math)
   - [Comparison Operators (Checking True or False)](#comparison-operators-checking-true-or-false)
   - [Logical Operators (`and`, `or`, `!`)](#logical-operators-and-or-)
   - [Shortcut Assignment Operators (`+=`, `-=`, etc.)](#shortcut-assignment-operators--)
5. [Strings & Text Manipulation](#5-strings--text-manipulation)
   - [String Indexing (Accessing Letters)](#string-indexing-accessing-letters)
   - [String Slicing `[start:end]`](#string-slicing-startend)
   - [Built-in String Methods & Properties](#built-in-string-methods--properties)
   - [Python-Style F-Strings (Formatted Text Interpolation)](#python-style-f-strings-formatted-text-interpolation)
6. [Collections (Storing Multiple Items)](#6-collections-storing-multiple-items)
   - [Fixed-Size Arrays (`<1, 2, 3>`)](#fixed-size-arrays-1-2-3)
   - [Dynamic Lists (`[1, 2, 3]`)](#dynamic-lists-1-2-3)
   - [Tuples (`(1, "A", true)`)](#tuples-1-a-true)
   - [Dictionaries (`{"key": "value"}`)](#dictionaries-key-value)
   - [Sets (`{1, 2, 3}`)](#sets-1-2-3)
   - [Sorted Lists (`sortedList(...)`)](#sorted-lists-sortedlist)
7. [Control Flow (Making Decisions & Repeating Steps)](#7-control-flow-making-decisions--repeating-steps)
   - [Conditional Decisions (`if`, `elif`, `else`)](#conditional-decisions-if-elif-else)
   - [Loops with `for ... in`](#loops-with-for--in)
   - [Loops with `while`](#loops-with-while)
   - [Loop Controls: `break` and `continue`](#loop-controls-break-and-continue)
8. [Functions (Reusable Code Recipes)](#8-functions-reusable-code-recipes)
   - [Bracketless Function Declarations (`f name { ... }`)](#bracketless-function-declarations-f-name----)
   - [The `takes(...)` Parameter Gateway](#the-takes-parameter-gateway)
   - [Variadic Functions (`args`)](#variadic-functions-args)
   - [Returning Values (`return`)](#returning-values-return)
   - [Global Built-in Functions](#global-built-in-functions)
9. [Object-Oriented Programming (Classes & Objects)](#9-object-oriented-programming-classes--objects)
   - [What is a Class?](#what-is-a-class)
   - [Fields & `this`](#fields--this)
   - [The Constructor (`init`)](#the-constructor-init)
   - [Class Methods](#class-methods)
   - [Creating & Using Instances](#creating--using-instances)
10. [Modules & Imports (Python-Style File Organization)](#10-modules--imports-python-style-file-organization)
    - [Basic Module Import (`import my_module`)](#basic-module-import-import-my_module)
    - [Import with Alias (`as`)](#import-with-alias-as)
    - [Selective Import (`from mod import item`)](#selective-import-from-mod-import-item)
    - [Wildcard Import (`from mod import *`)](#wildcard-import-from-mod-import-)
    - [Subfolders & Nested Modules (`sub.helper`)](#subfolders--nested-modules-subhelper)
11. [Standard Library (Built-In Modules)](#11-standard-library-built-in-modules)
    - [The `math` Module](#the-math-module)
    - [Python-Style File I/O (`open()`, `.read()`, `.write()`)](#python-style-file-io-open-read-write)
12. [Multi-Language Interoperability (Python, JS, C++, Java, Go, Rust)](#12-multi-language-interoperability)
13. [Foreign Function Interface (C FFI & `cimport`)](#13-foreign-function-interface-c-ffi--cimport)
14. [Dual Execution Architecture (How Skylang Runs Code)](#14-dual-execution-architecture)
15. [Error Handling & Python-Style Tracebacks](#15-error-handling--python-style-tracebacks)
16. [Automatic Garbage Collection](#16-automatic-garbage-collection)
17. [Comments (Writing Notes in Code)](#17-comments-writing-notes-in-code)
18. [VS Code Extension & Editor Superpowers](#18-vs-code-extension--editor-superpowers)
19. [License](#19-license)

---

## 1. Installation & Setup

### Prerequisites
Skylang compiles your code using native C tools. You only need standard development utilities:
- **GCC** (C11 compiler)
- **Make** (Build automation tool)
- **Boehm GC** (`libgc` / `bdw-gc` automatic memory library)
- **pkg-config**

### 🚀 One-Command Automatic Installation

Open your terminal and run:

```bash
git clone https://github.com/aakashdandekar/Skylang.git skylang_dev
cd skylang_dev
chmod +x install.sh
./install.sh
```

**What the installer does automatically for you:**
1. Detects your operating system (Ubuntu, Debian, Fedora, Arch Linux, macOS, etc.).
2. Installs any missing packages and compilers.
3. Builds the Skylang compiler and runtime library (`make clean all`).
4. Creates global terminal commands `sky` and `skylang` in `~/.local/bin`.
5. Installs the VS Code extension for full editor autocomplete and syntax highlighting.
6. Runs the test suite to ensure 100% functionality.

To uninstall at any time:
```bash
./delete.sh
```

---

## 2. Running Your First Program (CLI Usage)

Once installed, you can run Skylang programs using the `sky` command from any directory on your computer!

| Command | What it does | When to use it |
|---|---|---|
| `sky run <file.sky>` | Transpiles, compiles, and runs your program immediately in memory. | While writing and testing code day-to-day. |
| `sky build <file.sky> [-o <binary>]` | Compiles your program into a standalone, native executable file that runs anywhere without Skylang installed! | When you want to ship or distribute your final application. |
| `sky <file.sky>` | Short shortcut for `sky run <file.sky>`. | Quick runs. |

### Example: Running vs Building

Create a file named `hello.sky`:
```skylang
println("Welcome to the world of Skylang!")
```

**Run it instantly:**
```bash
sky run hello.sky
# Output: Welcome to the world of Skylang!
```

**Build a standalone binary:**
```bash
sky build hello.sky -o my_program
./my_program
# Output: Welcome to the world of Skylang!
```

---

## 3. Variables & Data Types (Storing Information)

### What is a Variable?
A **variable** is like a labeled storage box inside your computer's memory. You give the box a name, put some data inside it, and whenever you reference that name later, the computer looks inside the box and retrieves your data.

Skylang supports **12 fundamental types**:
- **Scalar (Single Value) Types**:
  - `int`: Whole numbers (`10`, `-42`, `1000`)
  - `double`: Decimal numbers (`3.14`, `-0.5`, `99.99`)
  - `bool`: True or False switches (`true`, `false`)
  - `char`: Single text characters (`'A'`, `'z'`, `'9'`)
  - `string`: Text passages (`"Hello, world!"`)
  - `type`: Data type descriptors
- **Collection (Group) Types**:
  - `array`: Fixed-size, super-fast list of items of the same type (`<1, 2, 3>`)
  - `list`: Dynamic, growing list of items of any type (`[1, "two", 3.0]`)
  - `tuple`: Fixed, unchangeable record (`(10, 20, "Alex")`)
  - `dict`: Key-value lookup dictionary (`{"name": "Alice", "age": 25}`)
  - `set`: Unique collection with no duplicates (`{1, 2, 3}`)
  - `sortedList`: Automatically sorted list (`sortedList([3, 1, 2])`)

---

### The Walrus Operator `:=` (Automatic Type Inference)

In Skylang, you don't have to manually tell the computer what type of data you are storing. By using the **walrus operator (`:=`)**, Skylang automatically detects what kind of information you are providing and sets up the storage box for you!

```skylang
score := 100              // Skylang knows this is an integer (int)
price := 19.99            // Skylang knows this is a decimal (double)
is_logged_in := true      // Skylang knows this is a boolean (bool)
initial := 'A'            // Skylang knows this is a character (char)
user_name := "Alice"      // Skylang knows this is a string (string)
numbers := [1, 2, 3, 4]   // Skylang knows this is a dynamic list (list)
```

---

### Explicit Type Prefixes (`I`, `D`, `B`, `C`, `S`)

If you prefer to be explicit, Skylang allows single-letter uppercase type prefixes:

| Prefix | Type | Example |
|---|---|---|
| `I` | Integer (Whole number) | `I age = 25` |
| `D` | Double (Decimal number) | `D temperature = 98.6` |
| `B` | Boolean (True/False) | `B is_active = true` |
| `C` | Character (Single letter) | `C grade = 'A'` |
| `S` | String (Text) | `S greeting = "Good morning"` |

---

### Default Values (Automatic Initialization)

In many languages (like C), if you declare a variable without giving it a value, it contains random garbage memory that causes bugs. In Skylang, **every uninitialized variable automatically receives a safe, sensible default value**:

```skylang
I total_count    // Automatically set to 0
D balance        // Automatically set to 0.0
B verified       // Automatically set to false
C first_letter   // Automatically set to 'a'
S user_bio       // Automatically set to "" (empty string)

println(total_count)  // Prints: 0
println(verified)     // Prints: false
```

---

### Checking Types with `.T`

Every single value and variable in Skylang has a special `.T` property. This allows you to inspect what type of data is inside at any time:

```skylang
x := 42
println(x.T)           // Prints: <type int>

name := "Skylang"
println(name.T)        // Prints: <type string>

// You can check types directly in if-conditions:
if x.T == "int" {
    println("x is indeed an integer!")
}
```

---

## 4. Operators (Doing Math & Logic)

Operators are special symbols that allow you to calculate, compare, or modify data.

### Arithmetic Operators (Math)

| Operator | Name | Example | Result | Explanation |
|---|---|---|---|---|
| `+` | Addition | `10 + 5` | `15` | Adds two numbers (or joins text) |
| `-` | Subtraction | `10 - 3` | `7` | Subtracts right number from left number |
| `*` | Multiplication | `4 * 6` | `24` | Multiplies two numbers |
| `/` | Division | `7.0 / 2.0` | `3.5` | Precise decimal division |
| `//` | Floor Division | `7 // 2` | `3` | Divides and rounds down to nearest whole number |
| `^` | Power (Exponent) | `2 ^ 8` | `256` | Raises 2 to the power of 8 ($2^8$) |
| `%` | Modulo (Remainder) | `10 % 3` | `1` | Calculates the leftover remainder after division |

```skylang
println(10 + 5)    // 15
println(10 // 3)   // 3 (10 divided by 3 is 3 with 1 left over)
println(10 % 3)    // 1 (the remainder)
println(2 ^ 4)     // 16 (2 * 2 * 2 * 2)
```

---

### Comparison Operators (Checking True or False)

Comparison operators compare two values and produce a boolean result (`true` or `false`):

| Operator | Meaning | Example | Result |
|---|---|---|---|
| `==` | Is equal to? | `5 == 5` | `true` |
| `!=` | Is NOT equal to? | `5 != 3` | `true` |
| `<` | Less than | `3 < 10` | `true` |
| `<=` | Less than or equal to | `5 <= 5` | `true` |
| `>` | Greater than | `10 > 20` | `false` |
| `>=` | Greater than or equal to | `8 >= 8` | `true` |

---

### Logical Operators (`and`, `or`, `!`)

Logical operators let you combine multiple true/false checks together:

- **`and` (or `&&`)**: Both conditions must be `true`.
- **`or` (or `||`)**: At least one condition must be `true`.
- **`!`**: Reverses the condition (`!true` becomes `false`).

```skylang
age := 22
has_id := true

if age >= 18 and has_id {
    println("Entry allowed!")
}

if age < 12 or age >= 65 {
    println("You qualify for a ticket discount!")
}
```

---

### Shortcut Assignment Operators (`+=`, `-=`, `*=`, `/=`)

Instead of writing `x = x + 5`, you can use handy shortcuts:

```skylang
I score = 10
score += 5    // Same as score = score + 5  (score is now 15)
score *= 2    // Same as score = score * 2  (score is now 30)
score -= 10   // Same as score = score - 10 (score is now 20)
println(score) // 20
```

---

## 5. Strings & Text Manipulation

A **string** is any sequence of text wrapped in double quotes (`"..."`) or single quotes (`'...'`).

```skylang
message := "Welcome to Skylang"
```

---

### String Indexing (Accessing Letters)

Computers start counting from `0`. In Skylang, you can access any character in a string using its 0-based index:

```skylang
lang := "Skylang"

println(lang[0])        // 'S' (The 1st character)
println(lang[1])        // 'k' (The 2nd character)
println(lang.value(2))  // 'y' (Using .value() method)
```

---

### String Slicing `[start:end]`

You can extract a slice of a string just like in Python!
- `[start:end]`: From index `start` up to (but not including) `end`.
- `[start:]`: From `start` all the way to the end.
- `[:end]`: From the beginning up to `end`.
- Negative indices count backwards from the end (`-1` is the last character).

```skylang
word := "Programming"

println(word[0:4])   // "Prog"
println(word[4:])    // "ramming"
println(word[-4:])   // "ming" (The last 4 characters)
```

---

### Built-in String Methods & Properties

Skylang comes equipped with an extensive suite of built-in string methods:

```skylang
text := "  Hello, Skylang World!  "

println(text.size)                 // 25 (Length of the string)
println(text.trim())               // "Hello, Skylang World!" (Removes extra spaces)
println(text.upper())              // "  HELLO, SKYLANG WORLD!  "
println(text.lower())              // "  hello, skylang world!  "
println(text.contains("Skylang"))  // true
println(text.startswith("  Hello"))// true
println(text.endswith("!  "))      // true
println(text.replace("World", "All")) // "  Hello, Skylang All!  "

// Splitting text into a list
fruits := "apple,banana,orange".split(",")
println(fruits)                    // ["apple", "banana", "orange"]

// Joining a list into text
joined := ", ".join(["Cat", "Dog", "Bird"])
println(joined)                    // "Cat, Dog, Bird"

// Reversing text
println("Skylang".reverse())       // "gnalykS"
```

---

### Python-Style F-Strings (Formatted Text Interpolation)

In many languages, combining text and variables looks messy:
`"Hello, " + name + "! You have " + count + " items."` (lots of plus signs and type conversions).

In Skylang, you can use **F-Strings** by prefixing your string with `f` (or `F`):
Inside `{...}`, you can write any variable, calculation, or function call!

```skylang
name := "Alice"
score := 95.5
items := ["Notebook", "Pen"]

// 1. Basic interpolation
println(f"Hello {name}, your score is {score}!")
// Output: Hello Alice, your score is 95.5!

// 2. Doing math calculations directly inside { ... }
println(f"Next level requires: {score + 10} points.")
// Output: Next level requires: 105.5 points.

// 3. Accessing collections and methods
println(f"Student {name.upper()} has {items.size} supplies: {items[0]} and {items[1]}.")
// Output: Student ALICE has 2 supplies: Notebook and Pen.

// 4. Escaped literal braces {{ and }}
println(f"To write a brace literally, use {{ and }}: {10 * 10}")
// Output: To write a brace literally, use { and }: 100
```

---

## 6. Collections (Storing Multiple Items)

When you need to store a list of names, coordinates, or user profiles, Skylang provides 6 specialized collection types.

### Fixed-Size Arrays (`<1, 2, 3>`)

An **Array** is a fixed-size, contiguous block of memory where every element has the **same type**. Because its size never changes, accessing it is lightning fast.

```skylang
// Declare an array of 4 integers (defaults to 0)
numbers I 4
numbers[0] = 10
numbers[1] = 20
numbers[2] = 30
numbers[3] = 40
println(numbers)      // <10, 20, 30, 40>

// Array literal using angle brackets <...>
primes I = <2, 3, 5, 7, 11>
println(primes.size)  // 5
```

---

### Dynamic Lists (`[1, 2, 3]`)

A **List** is a dynamic, resizable sequence that can grow, shrink, and hold **mixed types of data** (integers, strings, booleans, objects):

```skylang
// Create a dynamic list
shopping_list := ["Milk", "Bread", "Eggs"]

// Adding items with .push()
shopping_list.push("Butter")
println(shopping_list.size)   // 4

// Removing the last item with .pop()
last_item := shopping_list.pop()
println(last_item)            // "Butter"

// Slicing works on lists just like strings!
println(shopping_list[0:2])   // ["Milk", "Bread"]
```

---

### Tuples (`(1, "A", true)`)

A **Tuple** is an immutable (unchangeable) fixed sequence. Once created, its items cannot be modified, making it ideal for fixed coordinates or data records:

```skylang
point := (10, 20, "CenterPoint")

println(point[0])     // 10
println(point[1])     // 20
println(point[2])     // "CenterPoint"
println(point.size)   // 3
```

---

### Dictionaries (`{"key": "value"}`)

A **Dictionary** (or Hash Map) is like a real dictionary or phonebook: it maps unique **keys** to **values**:

```skylang
user := {
    "name": "Alice",
    "role": "Administrator",
    "score": 98.5,
    "active": true
}

// Accessing values by key
println(user["name"])        // "Alice"
println(user["role"])        // "Administrator"

// Adding or updating keys
user["city"] = "San Francisco"

// Checking if a key exists with .has()
if user.has("score") {
    println(f"Score is: {user[\"score\"]}")
}
```

---

### Sets (`{1, 2, 3}`)

A **Set** stores unique elements. If you try to add a duplicate item, the set automatically ignores it:

```skylang
unique_ids SET
unique_ids.add(101)
unique_ids.add(102)
unique_ids.add(101) // Duplicate!

println(unique_ids.size)     // 2 (only 101 and 102 exist)
println(unique_ids.has(102)) // true
```

---

### Sorted Lists (`sortedList(...)`)

A **Sorted List** automatically sorts its items whenever you insert new elements:

```skylang
scores SL
scores.add(50)
scores.add(10)
scores.add(90)
scores.add(30)

println(scores) // [10, 30, 50, 90] (Automatically in ascending order!)
```

---

## 7. Control Flow (Making Decisions & Repeating Steps)

### Conditional Decisions (`if`, `elif`, `else`)

Conditionals allow your program to take different paths depending on whether something is true or false:

```skylang
grade := 85

if grade >= 90 {
    println("Grade: A - Excellent!")
} elif grade >= 80 {
    println("Grade: B - Very Good!")
} elif grade >= 70 {
    println("Grade: C - Good")
} else {
    println("Grade: Needs Improvement")
}
```

---

### Loops with `for ... in`

A `for ... in` loop repeats a block of code for every item in a collection or range:

```skylang
// 1. Looping over a list
animals := ["Dog", "Cat", "Parrot"]
for animal in animals {
    println(f"Animal: {animal}")
}

// 2. Looping over a range of numbers (from 0 up to 5)
for i in range(5) {
    println(f"Step {i}")
}
```

---

### Loops with `while`

A `while` loop continues running as long as its condition remains `true`:

```skylang
countdown := 5

while countdown > 0 {
    println(f"T-minus {countdown}...")
    countdown -= 1
}
println("Liftoff! 🚀")
```

---

### Loop Controls: `break` and `continue`

- **`break`**: Immediately exits and terminates the entire loop.
- **`continue`**: Skips the rest of the current turn and jumps directly to the next item.

```skylang
for n in [1, 2, 3, 4, 5, 6, 7, 8] {
    if n == 3 {
        continue // Skip number 3
    }
    if n == 6 {
        break    // Stop the loop completely when we hit 6
    }
    println(n)
}
// Prints: 1, 2, 4, 5
```

---

## 8. Functions (Reusable Code Recipes)

A **function** is a reusable named block of instructions. Instead of writing the same 10 lines of code in multiple places, you write a function once and call it whenever you need it.

---

### Bracketless Function Declarations (`f name { ... }`)

In Skylang, function definitions are clean and **bracketless** — you do not need empty parentheses `()` in the definition header!

```skylang
f say_hello {
    println("Hello from a Skylang function!")
}

// Call the function:
say_hello()
```

---

### The `takes(...)` Parameter Gateway

When your function needs inputs (called **parameters**), place the `takes(...)` gateway at the very top of your function. This gives you automatic parameter validation and named argument support!

```skylang
f calculate_rectangle_area {
    takes(width, height)
    return width * height
}

// Call with regular positional arguments:
area1 := calculate_rectangle_area(10, 5)
println(area1) // 50

// Or call with crystal-clear named arguments in any order!
area2 := calculate_rectangle_area(height=20, width=5)
println(area2) // 100
```

---

### Variadic Functions (`args`)

If you don't know how many arguments someone will pass, omit `takes(...)`. Skylang automatically places all incoming arguments into a built-in `args` list:

```skylang
f sum_all {
    total := 0
    for num in args {
        total += num
    }
    return total
}

println(sum_all(1, 2, 3))          // 6
println(sum_all(10, 20, 30, 40))   // 100
```

---

### Returning Values (`return`)

A function can return a result (or multiple results) back to the caller:

```skylang
f min_max {
    takes(a, b)
    if a < b {
        return a, b
    } else {
        return b, a
    }
}

// Unpack multiple returned values:
smallest, largest := min_max(45, 12)
println(f"Smallest: {smallest}, Largest: {largest}")
// Output: Smallest: 12, Largest: 45
```

---

### Global Built-in Functions

Skylang comes with helpful global functions ready out of the box:

| Function | Description | Example |
|---|---|---|
| `print(...)` | Prints values to terminal without a newline at the end | `print("Hello ")` |
| `println(...)` | Prints values to terminal with an automatic newline | `println("Hello World")` |
| `input(prompt)` | Asks the user for text input in the terminal | `name := input("Enter name: ")` |
| `range(stop)` | Generates a sequence of numbers from 0 up to `stop - 1` | `for i in range(5)` |
| `len(x)` | Returns the size/length of any container or string | `len([1, 2, 3])` |
| `type(x)` | Returns the type object of any value | `type("text")` |
| `hex(num)` | Converts an integer to a hexadecimal string | `hex(255)` &rarr; `"ff"` |
| `bin(num)` | Converts an integer to a binary string | `bin(10)` &rarr; `"1010"` |
| `oct(num)` | Converts an integer to an octal string | `oct(64)` &rarr; `"100"` |
| `panic(msg)` | Halts the program immediately with an error message | `panic("Fatal error!")` |
| `eval(code)` | Dynamically evaluates and calculates Skylang code at runtime | `res := eval("10 + 20")` |

---

## 9. Object-Oriented Programming (Classes & Objects)

### What is a Class?
Think of a **Class** as a blueprint (like the blueprint for a House or a User Account). An **Object** (or instance) is the actual house built from that blueprint.

### Complete Class Example:

```skylang
class BankAccount {
    // 1. Declare instance fields (private by default)
    this.owner
    this.balance

    // 2. The constructor (called when creating a new account)
    init {
        takes(owner, initial_deposit)
        this.owner = owner
        this.balance = initial_deposit
    }

    // 3. Methods (actions this object can perform)
    deposit {
        takes(amount)
        this.balance += amount
        println(f"Deposited ${amount}. New balance: ${this.balance}")
    }

    withdraw {
        takes(amount)
        if amount > this.balance {
            println("Insufficient funds!")
            return false
        }
        this.balance -= amount
        println(f"Withdrew ${amount}. Remaining balance: ${this.balance}")
        return true
    }

    get_balance {
        return this.balance
    }
}

// Create an instance of the class:
account := BankAccount("Alice", 100)

// Call methods on the object:
account.deposit(50)     // Deposited $50. New balance: $150
account.withdraw(70)    // Withdrew $70. Remaining balance: $80
println(f"Final Balance: ${account.get_balance()}") // Final Balance: $80
```

---

## 10. Modules & Imports (Python-Style File Organization)

As your program grows, you'll want to split your code into multiple files. Skylang uses **Python-style import syntax**.

Imagine you have a file named `math_utils.sky`:
```skylang
// math_utils.sky
f square {
    takes(x)
    return x * x
}

PI := 3.14159
```

Here are the 4 ways you can import and use it in your `main.sky`:

### 1. Basic Module Import (`import my_module`)
```skylang
import math_utils

println(math_utils.square(5))  // 25
println(math_utils.PI)         // 3.14159
```

### 2. Import with Alias (`as`)
```skylang
import math_utils as mu

println(mu.square(8))          // 64
```

### 3. Selective Import (`from mod import item`)
```skylang
from math_utils import square, PI

println(square(6))             // 36
println(PI)                    // 3.14159
```

### 4. Wildcard Import (`from mod import *`)
```skylang
from math_utils import *

println(square(10))            // 100
```

### Subfolders & Nested Modules (`sub.helper`)
If your helper file is inside a subfolder `sub/helper.sky`:
```skylang
from sub.helper import greet_user

greet_user("Skylang Explorer")
```

---

## 11. Standard Library (Built-In Modules)

### The `math` Module
Skylang includes a built-in mathematical engine:

```skylang
println(math.sqrt(64))      // 8.0 (Square root)
println(math.pow(2, 8))     // 256.0 (2 to the power 8)
println(math.abs(-42))      // 42.0 (Absolute positive value)
println(math.min(10, 5))    // 5.0 (Minimum)
println(math.max(10, 5))    // 10.0 (Maximum)
println(math.floor(3.9))    // 3.0 (Round down)
println(math.ceil(3.1))     // 4.0 (Round up)
println(math.sin(0))        // 0.0 (Trigonometry)
println(math.pi)            // 3.141592653589793
println(math.e)             // 2.718281828459045
```

---

### Python-Style File I/O (`open()`, `.read()`, `.write()`)

Reading and writing files on your computer is simple and clean:

```skylang
// 1. Writing to a file
file := open("notes.txt", "w")
file.write("Line 1: Skylang is fun!\n")
file.write("Line 2: High performance and clean syntax.\n")
file.close()

// 2. Reading the entire file
reader := open("notes.txt", "r")
content := reader.read()
println("File Content:\n" + content)
reader.close()

// 3. Reading line by line
reader2 := open("notes.txt", "r")
lines := reader2.readlines()
reader2.close()

for line in lines {
    println(f"Read line: {line.trim()}")
}
```

---

## 12. Multi-Language Interoperability

Skylang's ultimate superpower is that it can **directly load and execute libraries written in Python, JavaScript/NPM, C++, Java, Go, and Rust without any messy setup**!

```skylang
import python, js, cpp, java, go, rust

// 1. Call Python's built-in math module
py_math := python.load("math")
println(py_math.sqrt(144))  // 12.0

// 2. Call JavaScript Math object
js_math := js.load("Math")
println(js_math.pow(3, 4))  // 81

// 3. JIT-Compile C++ code on the fly
cpp_calc := cpp.compile("
    #include <cmath>
    extern \"C\" double hypotenuse(double a, double b) {
        return std::sqrt(a*a + b*b);
    }
")
println(cpp_calc.hypotenuse(3.0, 4.0)) // 5.0

// 4. Call Java standard classes
java_math := java.load("java.lang.Math")
println(java_math.max(50, 99)) // 99

// 5. Call Go packages
go_math := go.load("math")
println(go_math.Sqrt(625.0))   // 25.0

// 6. JIT-Compile and call Rust functions
rust_calc := rust.compile("
    #[no_mangle]
    pub extern \"C\" fn cube(x: f64) -> f64 {
        x * x * x
    }
")
println(rust_calc.cube(4.0))   // 64.0
```

---

## 13. Foreign Function Interface (C FFI & `cimport`)

Because Skylang compiles to native C11, you can directly import native C headers and call C standard library functions with zero wrapper overhead!

```skylang
// Import standard C math header
cimport "math.h"

// Declare the external C function
extern f cos(x)
extern f sin(x)

println(cos(0.0)) // 1.0 (Direct C performance!)
```

---

## 14. Dual Execution Architecture

Skylang gives you two ways to run your code:

```
                  ┌───► AOT Codegen ───► C11 Code ───► GCC Compiler ───► Blazing Fast Native Binary
 .sky Source Code ┤
                  └───► Bytecode VM ───► Opcodes ───► Virtual Machine ──► Instant Run & Dynamic eval()
```

1. **AOT (Ahead-of-Time) Native Compilation (`sky build`)**:
   - Transpiles your code directly to optimized C11.
   - Compiles with GCC `-O2`.
   - Produces a single, standalone binary file with no runtime overhead.

2. **Bytecode Virtual Machine (`sky run`)**:
   - Compiles into memory-efficient bytecode opcodes.
   - Executes immediately with stack-based profiling and powers dynamic `eval("...")`.

---

## 15. Error Handling & Python-Style Tracebacks

Skylang uses **Go-style explicit error handling**:

```skylang
f divide {
    takes(a, b)
    if b == 0 {
        return error("Cannot divide by zero!")
    }
    return a / b
}

// Unpack result and error
result, err := divide(10, 0)

if err != none {
    println(f"Handled error safely: {err}")
} else {
    println(f"Success: {result}")
}
```

### Python-Style Call Stack Traceback
If an unexpected runtime crash happens (or `panic()` is triggered), Skylang prints a detailed traceback pointing to the exact file, line number, and function:

```text
Traceback (most recent call last):
  File "main.sky", line 14, in calculate_data
  File "main.sky", line 6, in divide
ZeroDivisionError: division by zero
```

---

## 16. Automatic Garbage Collection

In low-level languages like C or C++, you have to manually allocate and free every piece of memory (`malloc` / `free`). If you forget, your computer runs out of RAM (memory leak); if you free too early, your program crashes.

In Skylang, memory management is **100% automatic**. 
- Powered by the industrial-grade **Boehm-Demers-Weiser Garbage Collector** (`libgc`).
- As soon as your lists, strings, dictionaries, or objects are no longer in use, Skylang quietly recycles their memory in the background.

---

## 17. Comments (Writing Notes in Code)

Comments are notes written for humans that the computer completely ignores when running code:

```skylang
// This is a single-line comment

# This is also a single-line comment (Python style)

/* This is a 
   multi-line comment 
   spanning several lines */
```

---

## 18. VS Code Extension & Editor Superpowers

Skylang includes a first-class **Visual Studio Code Extension** designed to make writing code effortless:

- **One-Click Run**: Click the `$(play) Run` button in the top-right corner of any `.sky` file to execute instantly.
- **IntelliSense & Autocompletion**: Type `.` on any list, string, dictionary, or module (like `math.` or `python.`) to see all available methods with documentation.
- **Live Diagnostics**: Real-time red squiggly underlines catch syntax errors before you even run your program.
- **Smart Snippets**: Type `f`, `class`, `forin`, `fstr`, or `openread` and press `Tab` to generate complete code skeletons.

---

## 19. License

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
