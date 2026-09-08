# Skylang Programming Language: Complete Student Guide & Documentation

Welcome to **Skylang**, a compiled dynamic object-oriented programming language built upon C with automatic garbage collection.

This guide is designed for students learning Skylang from scratch. By the end of this tutorial, you will understand how to write, compile, run, and debug Skylang programs, as well as how Skylang translates into high-performance C code under the hood.

---

## Table of Contents
1. [Overview & Philosophy](#1-overview--philosophy)
2. [Installation & Toolchain](#2-installation--toolchain)
3. [Variables & Data Types](#3-variables--data-types)
4. [Operators](#4-operators)
5. [Strings, Collections & Slicing](#5-strings-collections--slicing)
6. [Control Flow](#6-control-flow)
7. [Functions & The takes(...) Feature](#7-functions--the-takes-feature)
8. [Object-Oriented Programming (Classes)](#8-object-oriented-programming-classes)
9. [Error Handling & Memory Management](#9-error-handling--memory-management)
10. [Student Practice Exercises](#10-student-practice-exercises)


---

## 1. Overview & Philosophy

Skylang combines the best attributes of modern languages:
- **Python-like Simplicity**: Clean syntax, slicing, dynamic typing, and expressive collections.
- **Go-like Predictability**: Unified `for` loop syntax and clean error reporting.
- **JavaScript-like Object Model**: Dynamic instance properties, method tables, and automatic garbage collection.
- **C Performance & Portability**: Skylang programs compile to C and link directly with GCC, generating native machine code executables.

---

## 2. Installation & Toolchain

### Building the Compiler
To build the Skylang compiler from source, run:
```bash
make clean && make
```
This produces two executable binaries in the `bin/` directory:
- `bin/skylang`: The full compiler executable.
- `bin/sky`: A convenient short alias.

### CLI Commands
| Command | Description | Example |
|---|---|---|
| `sky run <file.sky>` | Compiles and executes a program immediately | `./bin/sky run examples/01_basics.sky` |
| `sky build <file.sky> -o <bin>` | Compiles to a standalone native binary | `./bin/sky build main.sky -o my_app` |
| `sky repl` | Starts interactive shell | `./bin/sky repl` |

---

## 3. Variables & Data Types

Skylang supports 12 built-in types:
- Scalar Types: `int`, `double`, `bool`, `char`, `string`, `type`
- Collection Types: `array`, `list`, `tuple`, `dict`, `set`, `sortedList`

### Scalar Initializations & Default Values
In Skylang, you can declare variables using single uppercase type prefixes. Every scalar type has an automatic default value:

```skylang
I count       // int: default = 0
D temperature // double: default = 0.0
B active      // bool: default = false
C initial     // char: default = 'a'
S username    // string: default = ""
```

You can also initialize variables with values immediately:
```skylang
I age = 21
D pi = 3.14159
B is_logged_in = true
C grade = 'A'
S title = "Skylang Guide"
```

### The Walrus Operator (:=)
When you want Skylang to automatically infer the variable's type from its initial value, use the walrus operator `:=`:
```skylang
score := 95           // Inferred as int
rating := 4.8         // Inferred as double
message := "Success"  // Inferred as string
items := [1, 2, 3]    // Inferred as list
```

### Checking Types with .T
Every variable in Skylang has a `.T` property that returns its `type`:
```skylang
I x = 10
print(x.T)            // Prints: <type int>
print(x.T == "int")   // Prints: true
```

---

## 4. Operators

### Arithmetic Operators
| Operator | Description | Example | Result |
|---|---|---|---|
| `+` | Addition (or string/list concatenation) | `10 + 5` | `15` |
| `-` | Subtraction (or unary negation) | `10 - 4` | `6` |
| `*` | Multiplication (or string repetition) | `3 * 4` | `12` |
| `/` | Floating-point division | `10.0 / 4.0` | `2.5` |
| `//` | Floor / integer division | `10 // 3` | `3` |
| `^` | Exponentiation (power) | `2 ^ 4` | `16` |
| `%` | Modulo (remainder) | `10 % 3` | `1` |

### Comparison & Logical Operators
- Relational: `<`, `<=`, `>`, `>=`
- Equality: `==`, `!=`
- Logical: `and` (or `&&`), `or` (or `||`), `!` (not)
- Compound assignments: `+=`, `-=`, `*=`, `/=`

```skylang
I x = 10
x += 5 // x is now 15

if x > 10 and x < 20 {
    print("x is between 10 and 20")
}
```

---

## 5. Strings, Collections & Slicing

### Strings
Strings support indexing, slicing, `.size`, and `.value(index)`:
```skylang
S lang = "Skylang"
print("Length:", lang.size)       // 7
print("First char:", lang[0])      // S
print("Value at 1:", lang.value(1)) // k
print("Slice [0:3]:", lang[0:3])   // Sky
```

### Fixed-size Array vs Dynamic List
Skylang clearly separates fixed arrays from dynamic lists:

| Feature | Array | List |
|---|---|---|
| Size | Fixed capacity, contiguous in RAM | Dynamically resizable |
| Types | Homogeneous (same type) | Heterogeneous (mixed types) |
| Declaration | `arr I 5` or `arr I = <1, 2, 3>` | `lst L` or `lst = [1, 2, 3]` |
| Complexity | O(1) random access | O(1) append/pop, O(1) size access |

#### Fixed Array:
```skylang
// Declare array of int with 4 elements (defaults to 0)
numbers I 4
numbers[0] = 10
numbers[3] = 40
print("Array:", numbers) // <10, 0, 0, 40>

// Array literal with angle brackets <...>
primes I = <2, 3, 5, 7>
print("Primes:", primes)
```

#### Dynamic List:
```skylang
// Declare an empty list with postfix L
tasks L
tasks.push("Write code")
tasks.push("Run tests")
tasks.push("Deploy")

print("Size:", tasks.size) // 3
I last_item = tasks.pop()   // Removes "Deploy"
print("After pop:", tasks)

// List slicing works like Python:
numbers := [10, 20, 30, 40, 50]
print(numbers[1:4]) // [20, 30, 40]
```

### Tuple
Tuples are fixed, immutable sequences declared with `T` or parentheses:
```skylang
point T = (10, 20, "Origin")
print("Tuple item 0:", point[0])
print("Tuple size:", point.size)
```

### Dict (Dictionary / Hash Map)
Dictionaries map keys to values, declared with `DICT` or `{ key: value }`:
```skylang
scores DICT
scores["Alice"] = 95
scores["Bob"] = 88

print("Alice's score:", scores["Alice"])
print("Has Bob?", scores.has("Bob"))
print("Size:", scores.size)
```

### Set
Sets store unique elements, automatically discarding duplicates:
```skylang
tags SET
tags.add("coding")
tags.add("systems")
tags.add("coding") // Duplicate ignored

print("Tags:", tags)           // {systems, coding}
print("Has coding?", tags.has("coding")) // true
print("Size:", tags.size)      // 2
```

### SortedList
A SortedList automatically maintains all elements in ascending order upon insertion:
```skylang
grades SL
grades.push(85)
grades.push(42)
grades.push(99)
grades.push(70)

print(grades) // SL[42, 70, 85, 99]
```

---

## 6. Control Flow

### If, Elif, Else
Parentheses around conditions are optional. Use `elif` or `else if`:
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

### Unified For Loops (Go-Style)
Skylang unifies loops into a single, intuitive `for` construct:

#### 1. Infinite Loop with Break:
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

#### 2. Condition Loop (While):
```skylang
I i = 0
for i < 5 {
    print("i is:", i)
    i += 1
}
```

#### 3. Iterator Loop (`for var in collection`):
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

---

## 7. Functions, Default args, and The takes(...) Gateway

In Skylang, function and method definitions strictly do **not** have circular brackets:
```skylang
f func_name {
    // body
}
```

### Variadic by Default: The Implicit `args` List
By default, all functions and methods take an arbitrary number of arguments ("infinite args"). An implicit `args` list variable is automatically available inside every function and method:

```skylang
f add {
    I result
    for i in args {
        result += i
    }
    return result
}

print(add(1, 2, 3, 4, 5)) // 15
```

### The `takes(...)` Gateway & Named Arguments
When a function needs specific parameters, `takes(...)` acts as an explicit **gateway** that filters incoming arguments, binds declared parameter names, and enables both positional and named argument passing:

```skylang
f add {
    takes(a, b)
    return a + b
}

// 1. Positional arguments
print(add(1, 2)) // 3

// 2. Named arguments (order-independent)
print(add(a=1, b=2)) // 3
print(add(b=2, a=1)) // 3

// 3. Calling with existing initialized variables
a := 10
b := 20
print(add(a, b)) // 30
```

If an unknown named argument is passed that is not declared in `takes(...)`, the gateway rejects it with a runtime error.

### Recursion
Functions can be recursive:
```skylang
f fib {
    takes(n)
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

print("fib(10) =", fib(10)) // 55
```

---

## 8. Object-Oriented Programming (Classes)

Skylang features a complete class-based object-oriented system:
- Class fields are declared with `this.fieldName` and are private by default.
- Methods do **not** use circular brackets in their definitions (`method_name { ... }`).
- Constructors use the `init { ... }` block (which can use `takes(...)`).
- Method calls use circular brackets: `instance.method(...)`.

### Defining a Class
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
```

### Creating & Using Instances
```skylang
acc := BankAccount("Aakash", 1000)
acc.deposit(500)
acc.withdraw(200)

print("Owner:", acc.getOwner())      // Aakash
print("Balance:", acc.getBalance())  // 1300
```

---

## 9. Error Handling & Memory Management

### Go-Style Error Handling (`var, err := function()`)
In Skylang, you do **not** need to return `result, none` every time. A function's normal return statement is simply `return a`, and if no error occurs, the `err` variable is automatically set to `none` when unpacked!

To signal an error, return `error("error message")` or multiple values `return none, "error message"`:

```skylang
f safe_divide {
    takes(a, b)
    if b == 0 {
        return error("division by zero")
    }
    return a / b // Single return: err is automatically set to none!
}

// Successful call: err is automatically none
result, err := safe_divide(10, 2)
if err != none {
    print("Error encountered:", err)
} else {
    print("Result:", result) // Result: 5
}

// Error case: err captures the message, result is none
res, err2 := safe_divide(10, 0)
if err2 != none {
    print("Caught error:", err2) // Caught error: division by zero
}
```

### The Blank Identifier (`_`)
If you only care about the result and want to ignore the error (or vice versa), use the blank identifier `_`:
```skylang
ans, _ := safe_divide(100, 4)
print("Answer:", ans) // 25

_, err_only := safe_divide(10, 0)
print("Error was:", err_only)
```

### Multi-Variable Assignment & Swapping
You can also assign or swap multiple variables in a single statement:
```skylang
a, b := 10, 20
a, b = b, a // Swaps a and b: a is now 20, b is now 10
```

### Unrecoverable Errors (`panic`)
For unrecoverable situations where program execution cannot safely proceed, use the built-in `panic(message)`:
```skylang
if critical_system_failure {
    panic("Fatal: unable to initialize memory buffer")
}
```

### Built-in Functions
Skylang provides core built-in functions available everywhere without imports:

| Function | Signature | Description |
|---|---|---|
| `print(...)` | `print(a, b, ...)` | Prints arguments space-separated without trailing newline |
| `println(...)` | `println(a, b, ...)` | Prints arguments space-separated with a trailing newline |
| `eval(expr)` | `eval(code_str)` | Dynamically evaluates Skylang expressions at runtime and returns a Value |
| `range(stop)` | `range(stop)` / `range(start, stop, step)` | Returns a list of sequential integers |
| `len(coll)` | `len(coll)` | Returns the item count or string length |
| `type(val)` | `type(val)` | Returns the type descriptor object |
| `takes(...)` | `takes(p1, p2, ...)` | Declares parameter filter gateway in functions and methods |
| `error(msg)` | `error(message)` | Constructs an error object for Go-style returns |
| `panic(msg)` | `panic(message)` | Halts execution immediately |
| `gc()` | `gc()` | Triggers an immediate garbage collection cycle |
| `free(obj)` | `free(obj)` | Explicitly frees an object's memory |

### Multi-Language Interoperability
Skylang connects directly to other languages:
- **Python (`import python`)**: `python.load("math")`, `python.exec("...")`
- **JavaScript & NPM (`import js`)**: `js.load("Math")` (globals) & `js.load("lodash")` (NPM packages), `js.exec("...")`
- **C++ (`import cpp`)**: `cpp.compile("...")`, `cpp.load("./lib.so")`
- **Java (`import java`)**: `java.load("java.lang.Math")`, `java.exec("...")`
- **Golang (`import go`)**: `go.load("math")`, `go.compile("...")`, `go.exec("...")`
- **Rust (`import rust`)**: `rust.load("std::f64::consts")`, `rust.compile("...")`, `rust.exec("...")`

```skylang
import python, js, go, rust

// Dynamic Skylang Expression Evaluation
ans := eval("10 * 20 + 56")
println("eval result:", ans) // 256

// Calling Foreign Bridges
py_math := python.load("math")
println("Python sqrt:", py_math.sqrt(256))

js_math := js.load("Math")
println("JS max:", js_math.max(10, 50, 99))

go_math := go.load("math")
println("Go Sqrt:", go_math.Sqrt(144.0))

rust_math := rust.load("std::f64::consts")
println("Rust PI:", rust_math.PI)
```

---

## 10. Student Practice Exercises

### Exercise 1: Temperature Converter
Write a program that converts Celsius to Fahrenheit using formula `F = (C * 9/5) + 32`.
```skylang
f to_fahrenheit {
    takes(celsius)
    return (celsius * 9.0 / 5.0) + 32.0
}
print("0 C in F:", to_fahrenheit(0.0))    // 32.0
print("100 C in F:", to_fahrenheit(100.0)) // 212.0
```

### Exercise 2: Word Frequency Counter
Use a `DICT` to count how many times each word appears in a list.
```skylang
words := ["apple", "banana", "apple", "cherry", "banana", "apple"]
counts DICT

for word in words {
    if counts.has(word) {
        counts[word] += 1
    } else {
        counts[word] = 1
    }
}

print("Frequency table:", counts)
```

### Exercise 3: Car Class
Create a `Car` class with fields `this.make`, `this.model`, and `this.mileage`, with a method `drive` that increases `this.mileage`.
```skylang
class Car {
    this.make
    this.model
    this.mileage

    init {
        takes(make, model)
        this.make = make
        this.model = model
        this.mileage = 0
    }

    drive {
        takes(miles)
        this.mileage += miles
    }

    info {
        print(this.make + " " + this.model + " - Mileage: " + this.mileage)
    }
}

my_car := Car("Tesla", "Model 3")
my_car.drive(150)
my_car.info()
```

Happy coding with Skylang!
