"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.CONTAINER_TYPE_MEMBERS = exports.PROPERTIES = exports.COLLECTION_METHODS = exports.BUILTIN_FUNCTIONS = void 0;
exports.BUILTIN_FUNCTIONS = {
    'print': {
        name: 'print',
        signature: 'print(arg1, arg2, ...)',
        description: 'Prints arguments space-separated to standard output without a trailing newline.',
        params: [{ name: '...args', doc: 'Values to print' }],
        returns: 'none',
        example: 'print("Count:", 42)'
    },
    'println': {
        name: 'println',
        signature: 'println(arg1, arg2, ...)',
        description: 'Prints arguments space-separated to standard output followed by a trailing newline.',
        params: [{ name: '...args', doc: 'Values to print' }],
        returns: 'none',
        example: 'println("Hello, Skylang!")'
    },
    'eval': {
        name: 'eval',
        signature: 'eval(code_str)',
        description: 'Dynamically evaluates a Skylang expression string at runtime and returns the resulting Value.',
        params: [{ name: 'code_str', doc: 'Skylang expression string to evaluate' }],
        returns: 'any',
        example: 'val := eval("10 * 20 + 56")\nprintln("Result:", val) // 256'
    },
    'range': {
        name: 'range',
        signature: 'range(stop) or range(start, stop[, step])',
        description: 'Generates a dynamic list containing an arithmetic sequence progression.',
        params: [
            { name: 'start', doc: 'Starting integer (optional, default 0)' },
            { name: 'stop', doc: 'Upper bound integer (exclusive)' },
            { name: 'step', doc: 'Step increment (optional, default 1)' }
        ],
        returns: 'list',
        example: 'for i in range(10) {\n    println("Index:", i)\n}\nfor evens in range(0, 20, 2) {\n    println(evens)\n}'
    },
    'len': {
        name: 'len',
        signature: 'len(collection)',
        description: 'Returns the number of elements in an array, list, tuple, dictionary, set, sorted list, or the length of a string.',
        params: [{ name: 'collection', doc: 'The container or string' }],
        returns: 'int',
        example: 'n := len(my_list)\ns_len := len("Skylang")'
    },
    'type': {
        name: 'type',
        signature: 'type(val)',
        description: 'Returns the runtime type descriptor object of the given value (equivalent to `val.T`).',
        params: [{ name: 'val', doc: 'Value to inspect' }],
        returns: 'type',
        example: 'if type(x) == "int" {\n    println("x is an integer")\n}'
    },
    'takes': {
        name: 'takes',
        signature: 'takes(param1, param2, ...)',
        description: 'Parameter gateway for functions and class constructors. Binds positional and named arguments to local variables and rejects unknown argument names.',
        params: [{ name: '...params', doc: 'Allowed parameter variable names' }],
        returns: 'none',
        example: 'f add {\n    takes(a, b)\n    return a + b\n}\n\nprintln(add(a=10, b=20))'
    },
    'error': {
        name: 'error',
        signature: 'error(message)',
        description: 'Constructs an error object for Go-style error handling.',
        params: [{ name: 'message', doc: 'Error message description string' }],
        returns: 'error',
        example: 'f safe_div {\n    takes(a, b)\n    if b == 0 {\n        return error("division by zero")\n    }\n    return a / b\n}'
    },
    'panic': {
        name: 'panic',
        signature: 'panic(message)',
        description: 'Terminates program execution immediately and prints a fatal error message.',
        params: [{ name: 'message', doc: 'Fatal error explanation' }],
        returns: 'never',
        example: 'if critical_failure {\n    panic("Fatal: unable to allocate memory")\n}'
    },
    'gc': {
        name: 'gc',
        signature: 'gc()',
        description: 'Manually triggers an immediate Boehm-Demers-Weiser GC garbage collection cycle.',
        returns: 'none',
        example: 'gc() // trigger full collection'
    },
    'free': {
        name: 'free',
        signature: 'free(obj)',
        description: 'Explicitly deallocates memory for a specific heap-allocated object.',
        params: [{ name: 'obj', doc: 'Object to deallocate' }],
        returns: 'none',
        example: 'free(heavy_buffer)'
    }
};
exports.COLLECTION_METHODS = {
    'push': {
        name: 'push',
        signature: 'list.push(item) / sortedList.push(item)',
        description: 'Appends an item to the end of a dynamic list, or inserts it in ascending sorted order for a sorted list (`SL`).',
        params: [{ name: 'item', doc: 'The element to insert' }],
        returns: 'none',
        example: 'tasks.push("Write code")\ngrades.push(95)',
        container: 'list | sortedList'
    },
    'pop': {
        name: 'pop',
        signature: 'list.pop([index]) / sortedList.pop([index])',
        description: 'Removes and returns the last element, or the element at the specified 0-based index.',
        params: [{ name: 'index', doc: 'Optional index to pop from (default is last item)' }],
        returns: 'element',
        example: 'last := tasks.pop()\nfirst := tasks.pop(0)',
        container: 'list | sortedList'
    },
    'clear': {
        name: 'clear',
        signature: 'list.clear()',
        description: 'Removes all elements from the dynamic list, resetting its size to 0.',
        returns: 'none',
        example: 'tasks.clear()',
        container: 'list'
    },
    'add': {
        name: 'add',
        signature: 'set.add(item)',
        description: 'Adds an element to the set. If the element already exists, the operation is ignored.',
        params: [{ name: 'item', doc: 'The item to add' }],
        returns: 'none',
        example: 'tags.add("skylang")\ntags.add("compiler")',
        container: 'set'
    },
    'has': {
        name: 'has',
        signature: 'dict.has(key) / set.has(item)',
        description: 'Returns `true` if the key exists in the dictionary or the item exists in the set, otherwise returns `false`.',
        params: [{ name: 'key_or_item', doc: 'Key or item to test for existence' }],
        returns: 'bool',
        example: 'if scores.has("Alice") {\n    println("Alice found!")\n}\nif tags.has("coding") {\n    ...\n}',
        container: 'dict | set'
    },
    'value': {
        name: 'value',
        signature: 'str.value(index)',
        description: 'Returns the single character at the specified 0-based index in the string.',
        params: [{ name: 'index', doc: 'Character index (0-based)' }],
        returns: 'char',
        example: 'ch := name.value(0) // \'S\'',
        container: 'string'
    }
};
exports.PROPERTIES = {
    'size': {
        name: 'size',
        signature: 'container.size / string.size',
        description: 'O(1) property returning the number of elements in an array, list, tuple, dictionary, set, sortedList, or length of a string.',
        returns: 'int',
        example: 'println("List size:", tasks.size)\nprintln("String length:", name.size)\nprintln("Dict entries:", scores.size)'
    },
    'T': {
        name: 'T',
        signature: 'val.T',
        description: 'Returns the runtime type descriptor of the variable (e.g. `<type int>`, `<type string>`, `<type list>`).',
        returns: 'type',
        example: 'if x.T == "int" {\n    println("x is an integer")\n}\nprintln("Type:", tasks.T)'
    }
};
exports.CONTAINER_TYPE_MEMBERS = {
    'list': {
        methods: ['push', 'pop', 'clear'],
        properties: ['size', 'T']
    },
    'sortedList': {
        methods: ['push', 'pop'],
        properties: ['size', 'T']
    },
    'dict': {
        methods: ['has'],
        properties: ['size', 'T']
    },
    'set': {
        methods: ['add', 'has'],
        properties: ['size', 'T']
    },
    'string': {
        methods: ['value'],
        properties: ['size', 'T']
    },
    'array': {
        methods: [],
        properties: ['size', 'T']
    },
    'tuple': {
        methods: [],
        properties: ['size', 'T']
    }
};
//# sourceMappingURL=builtins.js.map