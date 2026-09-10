export interface KeywordDoc {
    name: string;
    description: string;
    syntax: string;
    example?: string;
    category?: string;
}

export const KEYWORDS: Record<string, KeywordDoc> = {
    'f': {
        name: 'f (Function Declaration)',
        syntax: 'f function_name { ... }',
        description: 'Declares a function in Skylang. Function definitions are strictly bracketless (no parentheses after the function name). Inside the function body, parameters can be accepted variadically via the implicit `args` list or filtered using the `takes(...)` gateway.',
        example: 'f add {\n    takes(a, b)\n    return a + b\n}',
        category: 'Declaration'
    },
    'takes': {
        name: 'takes (Parameter Gateway)',
        syntax: 'takes(param1, param2, ...)',
        description: 'Explicit parameter filtering gateway placed at the top of a function or class `init` method. Binds incoming positional or named arguments to local variables, enforces parameter validation, and rejects undeclared named parameters.',
        example: 'f greet {\n    takes(name, title)\n    println("Hello,", title, name)\n}',
        category: 'Function'
    },
    'return': {
        name: 'return (Return Statement)',
        syntax: 'return [expression] | return val1, val2',
        description: 'Returns a value or multiple values from a function. For Go-style error handling, returning a single value automatically sets `err` to `none` when unpacked as `res, err := fn()`. To signal an error, return `error("message")`.',
        example: 'return result\n// or error:\nreturn error("not found")',
        category: 'Control Flow'
    },
    'class': {
        name: 'class (Class Declaration)',
        syntax: 'class ClassName { ... }',
        description: 'Defines an Object-Oriented class in Skylang. Class fields are declared at the top using `this.fieldName` and are private by default. Constructors are defined using `init { ... }`. Methods do not use the `f` keyword.',
        example: 'class Dog {\n    this.name\n    this.breed\n\n    init {\n        takes(name, breed)\n        this.name = name\n        this.breed = breed\n    }\n\n    bark {\n        println(this.name, "says: Woof!")\n    }\n}',
        category: 'OOP'
    },
    'init': {
        name: 'init (Class Constructor)',
        syntax: 'init { takes(...) ... }',
        description: 'The constructor block of a Skylang class. Automatically executed when creating a new instance via `ClassName(args...)` or `new ClassName(args...)`. Uses `takes(...)` to declare constructor parameters.',
        example: 'init {\n    takes(name, initial_balance)\n    this.name = name\n    this.balance = initial_balance\n}',
        category: 'OOP'
    },
    'this': {
        name: 'this (Instance Self Reference)',
        syntax: 'this.fieldName or this.methodName(...)',
        description: 'References the current class instance fields and methods inside a class definition. Fields declared with `this.fieldName` are private to the class.',
        example: 'this.balance += amount\nreturn this.balance',
        category: 'OOP'
    },
    'if': {
        name: 'if (Conditional Branch)',
        syntax: 'if condition { ... }',
        description: 'Executes a block of code if the condition evaluates to truthy. Curly braces `{}` are required. Parentheses around the condition are optional.',
        example: 'if score >= 90 {\n    println("Grade: A")\n}',
        category: 'Control Flow'
    },
    'elif': {
        name: 'elif (Else-If Branch)',
        syntax: 'elif condition { ... }',
        description: 'Tests an alternate condition if preceding `if` or `elif` checks evaluate to falsy.',
        example: 'elif score >= 80 {\n    println("Grade: B")\n}',
        category: 'Control Flow'
    },
    'else': {
        name: 'else (Default Fallback Branch)',
        syntax: 'else { ... }',
        description: 'Executes when all preceding `if` and `elif` conditions evaluate to falsy.',
        example: 'else {\n    println("Grade: F")\n}',
        category: 'Control Flow'
    },
    'for': {
        name: 'for (Unified Loop)',
        syntax: 'for { ... } | for condition { ... } | for item in collection { ... }',
        description: 'Skylang unified loop statement (Go-style). Unifies all looping patterns into a single keyword:\n1. Infinite loop: `for { ... }`\n2. Condition loop (while): `for condition { ... }`\n3. Iterator loop: `for item in collection { ... }`',
        example: '// 1. Iterator:\nfor item in items {\n    println(item)\n}\n// 2. Condition:\nfor count < 10 {\n    count += 1\n}\n// 3. Infinite:\nfor {\n    if done { break }\n}',
        category: 'Control Flow'
    },
    'in': {
        name: 'in (Loop Iterator / Range Membership)',
        syntax: 'for item in collection',
        description: 'Used with the `for` keyword to iterate over dynamic lists, fixed arrays, strings, tuples, dictionaries, sets, sorted lists, or `range(...)` progressions.',
        example: 'for i in range(10) {\n    println("Index:", i)\n}',
        category: 'Control Flow'
    },
    'break': {
        name: 'break (Loop Termination)',
        syntax: 'break',
        description: 'Terminates execution of the innermost enclosing `for` loop immediately.',
        example: 'if found { break }',
        category: 'Control Flow'
    },
    'continue': {
        name: 'continue (Next Loop Iteration)',
        syntax: 'continue',
        description: 'Skips the remaining statements in the current iteration of the innermost enclosing `for` loop and begins the next iteration.',
        example: 'if item == 0 { continue }',
        category: 'Control Flow'
    },
    'import': {
        name: 'import (Module & Language Bridge Import)',
        syntax: 'import module [as alias] | import mod1, mod2 | import python as py',
        description: 'Imports local `.sky` modules, subpackages, or multi-language interoperability bridges:\n- **Module Import**: `import math_utils` or `import sub.helper as sh`\n- **Aliasing**: `import module as alias`\n- **Multi-Import**: `import math_utils as mu, utils`\n- **Language Bridges**: `import python`, `import js`, `import cpp`, `import java`, `import go`, `import rust`',
        example: 'import math_utils as mu\nprintln("Square:", mu.square(10))\n\nimport python as py\nmath_py := py.load("math")',
        category: 'Module'
    },
    'from': {
        name: 'from (Selective Module Import)',
        syntax: 'from module import symbol1, symbol2 [as alias] | from module import *',
        description: 'Selectively imports functions, classes, and constants from a `.sky` file module or language bridge into the current scope.\n- **Specific symbols**: `from math_utils import add, multiply`\n- **Symbol aliasing**: `from math_utils import square as sq`\n- **Wildcard import**: `from math_utils import *` (imports all exported symbols)',
        example: 'from math_utils import add, multiply as mult\nprintln("Sum:", add(10, 20))\n\nfrom math_utils import *\nprintln("PI:", PI)',
        category: 'Module'
    },
    'as': {
        name: 'as (Import Aliasing)',
        syntax: 'import module as alias | from module import symbol as alias',
        description: 'Assigns an alias to an imported module, language bridge, or individual imported symbol to prevent naming collisions and simplify usage.',
        example: 'import math_utils as mu\nfrom sub.helper import greet as say_hi\nimport python as py',
        category: 'Module'
    },
    'cimport': {
        name: 'cimport (C Foreign Function Interface)',
        syntax: 'cimport "header.h"',
        description: 'Imports a C header file for direct C interoperability via Foreign Function Interface (FFI). Declared functions can be called directly as native C code.',
        example: 'cimport "math.h"\n\nextern f cos(x)\nextern f sin(x)\nextern f sqrt(x)\n\nprintln("C cos(0):", cos(0.0))',
        category: 'FFI'
    },
    'extern': {
        name: 'extern (C External Function Declaration)',
        syntax: 'extern f function_name(param1, ...)',
        description: 'Declares an external C function signature imported via `cimport` for native binding in Skylang.',
        example: 'extern f atan2(y, x)\nextern f pow(base, exp)',
        category: 'FFI'
    },
    'new': {
        name: 'new (Object Instantiation)',
        syntax: 'new ClassName(args...)',
        description: 'Instantiates a class. Note: `new` is optional in Skylang; `ClassName(args...)` can also be called directly to construct an object.',
        example: 'account := new BankAccount("Alice", 1000)',
        category: 'OOP'
    },
    'and': {
        name: 'and (Logical AND Operator)',
        syntax: 'expr1 and expr2 (or expr1 && expr2)',
        description: 'Short-circuiting logical AND operator. Evaluates to true only if both operands evaluate to truthy.',
        example: 'if x > 0 and x < 100 {\n    println("In range")\n}',
        category: 'Operator'
    },
    'or': {
        name: 'or (Logical OR Operator)',
        syntax: 'expr1 or expr2 (or expr1 || expr2)',
        description: 'Short-circuiting logical OR operator. Evaluates to true if either operand evaluates to truthy.',
        example: 'if is_admin or is_superuser {\n    grant_access()\n}',
        category: 'Operator'
    },
    'async': {
        name: 'async (Asynchronous Function Modifier)',
        syntax: 'async f function_name { ... }',
        description: 'Declares an asynchronous function. Invoking it immediately returns a Future object without blocking the caller.',
        example: 'async f fetch_data {\n    takes(id)\n    async.sleep(0.5)\n    return {"id": id, "data": "OK"}\n}\ntask := fetch_data(1)\nresult := await task',
        category: 'Concurrency'
    },
    'await': {
        name: 'await (Await Future Resolution)',
        syntax: 'await future_expression',
        description: 'Unary operator that pauses execution until the given Future completes and yields its resolved value (or raises an error).',
        example: 'result := await async_operation()',
        category: 'Concurrency'
    },
    'spawn': {
        name: 'spawn (Spawn Background Worker)',
        syntax: 'spawn function_call()',
        description: 'Spawns execution of a regular function or expression onto a background worker thread, immediately returning a Future.',
        example: 'fut := spawn heavy_compute(1000)\nres := await fut',
        category: 'Concurrency'
    }
};

export const TYPES: Record<string, KeywordDoc> = {
    'I': {
        name: 'I (int - 64-bit Signed Integer)',
        syntax: 'I var_name [= initial_value] | arr_name I size',
        description: '64-bit signed integer scalar type. Automatic default value is `0`. Also used for fixed-size homogeneous array declarations (`numbers I 5`).',
        example: 'I count          // default 0\nI age = 21       // initialized\nprimes I = <2, 3, 5, 7> // array literal',
        category: 'Scalar Type'
    },
    'D': {
        name: 'D (double - 64-bit IEEE 754 Floating Point)',
        syntax: 'D var_name [= initial_value]',
        description: '64-bit IEEE 754 floating point double scalar type. Automatic default value is `0.0`.',
        example: 'D temperature    // default 0.0\nD pi = 3.14159265 // initialized',
        category: 'Scalar Type'
    },
    'B': {
        name: 'B (bool - Boolean)',
        syntax: 'B var_name [= initial_value]',
        description: 'Boolean scalar type (`true` or `false`). Automatic default value is `false`.',
        example: 'B is_active      // default false\nB is_ready = true // initialized',
        category: 'Scalar Type'
    },
    'C': {
        name: 'C (char - Character)',
        syntax: 'C var_name [= initial_value]',
        description: 'Single ASCII/character scalar type. Automatic default value is `\'a\'`.',
        example: 'C initial        // default \'a\'\nC grade = \'A\'     // initialized',
        category: 'Scalar Type'
    },
    'S': {
        name: 'S (string - UTF-8 String)',
        syntax: 'S var_name [= initial_value]',
        description: 'UTF-8 string scalar type with O(1) `.size`, `.value(i)` indexing, and Python-style slicing `[start:end]`. Automatic default value is `""` (empty string).',
        example: 'S title          // default ""\nS name = "Skylang"\nprintln(name[0:3]) // "Sky"\nprintln(name.size) // 7',
        category: 'Scalar Type'
    },
    'L': {
        name: 'L (list - Dynamic Resizable List)',
        syntax: 'list_name L | items := [1, 2, 3]',
        description: 'Dynamic, resizable, heterogeneous collection. Supports `.push()`, `.pop()`, `.clear()`, O(1) `.size`, and Python-style slicing `[start:end]`.',
        example: 'tasks L\ntasks.push("Write code")\ntasks.push("Run tests")\nprintln("Size:", tasks.size) // 2\npopped := tasks.pop() // "Run tests"',
        category: 'Collection Type'
    },
    'T': {
        name: 'T (tuple - Fixed Immutable Sequence / Type Property)',
        syntax: 'tuple_name T = (val1, val2, ...) | val.T',
        description: 'Fixed-size immutable sequence. Elements cannot be modified after creation. Also used as the `.T` property on any variable to query its runtime type descriptor.',
        example: 'point T = (10, 20, "Origin")\nprintln(point[0]) // 10\nprintln(point.size) // 3\n\n// Type check:\nI x = 42\nprintln(x.T == "int") // true',
        category: 'Collection Type'
    },
    'DICT': {
        name: 'DICT (Dictionary / Hash Map)',
        syntax: 'dict_name DICT | config := {"key": "val", 1: 100}',
        description: 'Hash map key-value store. Supports `dict[key]`, `dict[key] = val`, `.has(key)` existence check, and O(1) `.size`.',
        example: 'scores DICT\nscores["Alice"] = 95\nscores["Bob"] = 88\nif scores.has("Alice") {\n    println("Alice score:", scores["Alice"])\n}',
        category: 'Collection Type'
    },
    'SET': {
        name: 'SET (Set - Unique Element Collection)',
        syntax: 'set_name SET',
        description: 'Collection of unique elements where duplicates are automatically discarded. Supports `.add()`, `.has()`, and O(1) `.size`.',
        example: 'tags SET\ntags.add("coding")\ntags.add("systems")\ntags.add("coding") // ignored\nprintln("Size:", tags.size) // 2',
        category: 'Collection Type'
    },
    'SL': {
        name: 'SL (Sorted List - Auto-Sorted Ascending Collection)',
        syntax: 'sl_name SL',
        description: 'Automatically maintained ascending sorted list. Elements inserted via `.push(item)` are automatically placed in their sorted positions.',
        example: 'grades SL\ngrades.push(85)\ngrades.push(42)\ngrades.push(99)\nprintln(grades) // SL[42, 85, 99]',
        category: 'Collection Type'
    },
    'int': {
        name: 'int (Type Descriptor)',
        syntax: 'val.T == "int" or type(val) == "int"',
        description: 'Integer type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'double': {
        name: 'double (Type Descriptor)',
        syntax: 'val.T == "double" or type(val) == "double"',
        description: 'Double floating point type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'bool': {
        name: 'bool (Type Descriptor)',
        syntax: 'val.T == "bool" or type(val) == "bool"',
        description: 'Boolean type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'char': {
        name: 'char (Type Descriptor)',
        syntax: 'val.T == "char" or type(val) == "char"',
        description: 'Character type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'string': {
        name: 'string (Type Descriptor)',
        syntax: 'val.T == "string" or type(val) == "string"',
        description: 'String type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'list': {
        name: 'list (Type Descriptor)',
        syntax: 'val.T == "list" or type(val) == "list"',
        description: 'Dynamic list type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'array': {
        name: 'array (Type Descriptor)',
        syntax: 'val.T == "array" or type(val) == "array"',
        description: 'Fixed array type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'tuple': {
        name: 'tuple (Type Descriptor)',
        syntax: 'val.T == "tuple" or type(val) == "tuple"',
        description: 'Tuple type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'dict': {
        name: 'dict (Type Descriptor)',
        syntax: 'val.T == "dict" or type(val) == "dict"',
        description: 'Dictionary type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'set': {
        name: 'set (Type Descriptor)',
        syntax: 'val.T == "set" or type(val) == "set"',
        description: 'Set type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'sortedList': {
        name: 'sortedList (Type Descriptor)',
        syntax: 'val.T == "sortedList" or type(val) == "sortedList"',
        description: 'SortedList type identifier descriptor returned by `.T` or `type()`.',
        category: 'Type Name'
    },
    'future': {
        name: 'future (Future / Asynchronous Promise Object)',
        syntax: 'task := async_fn() | fut := spawn fn()',
        description: 'First-class Future concurrency object representing an asynchronous computation. Supports `.await()`, `.is_done`, `.result`, `.error`, `.state`, `.cancel()`.',
        example: 'fut := async.sleep(1.0)\nawait fut',
        category: 'Concurrency'
    },
    'type': {
        name: 'type (Type Descriptor)',
        syntax: 'val.T',
        description: 'Runtime type descriptor object representing a Skylang data type.',
        category: 'Type Name'
    }
};

export const CONSTANTS: Record<string, KeywordDoc> = {
    'true': {
        name: 'true (Boolean Literal)',
        syntax: 'true',
        description: 'Boolean literal representing truth.'
    },
    'false': {
        name: 'false (Boolean Literal)',
        syntax: 'false',
        description: 'Boolean literal representing falsehood.'
    },
    'nil': {
        name: 'nil / none / None (Null Value)',
        syntax: 'nil | none | None',
        description: 'Represents the absence of a value or null object in Skylang.'
    },
    'none': {
        name: 'none (Go-Style Default Null / No-Error)',
        syntax: 'none',
        description: 'Skylang null/nil value, automatically returned for `err` when a function returns a normal single value in Go-style multiple returns.',
        example: 'res, err := safe_fn()\nif err != none {\n    println("Error:", err)\n}'
    },
    'None': {
        name: 'None (Python / Skylang Null Literal)',
        syntax: 'None',
        description: 'Alias for Skylang null/nil value.'
    },
    'args': {
        name: 'args (Implicit Function Argument List)',
        syntax: 'args',
        description: 'Implicit dynamic list automatically available inside any function or method that does not use `takes(...)`. Contains all incoming arguments variadically.',
        example: 'f sum_all {\n    I total\n    for i in args {\n        total += i\n    }\n    return total\n}'
    },
    '_': {
        name: '_ (Blank Identifier)',
        syntax: '_',
        description: 'Blank identifier used to discard unneeded return values or variables in destructuring assignments.',
        example: 'val, _ := safe_divide(100, 4) // ignore error\n_, err := safe_divide(10, 0)   // ignore result'
    }
};
