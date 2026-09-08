export interface BuiltinDoc {
    name: string;
    signature: string;
    description: string;
    params?: { name: string; doc: string }[];
    returns?: string;
    example?: string;
    container?: string;
}

export const BUILTIN_FUNCTIONS: Record<string, BuiltinDoc> = {
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
    'open': {
        name: 'open',
        signature: 'open(filepath, [mode])',
        description: 'Opens a file and returns a file object (Python-style). Modes: "r" (read), "w" (write), "a" (append), "rb", "wb".',
        params: [
            { name: 'filepath', doc: 'Path to the target file on disk' },
            { name: 'mode', doc: 'Optional access mode string: "r", "w", "a" (default is "r")' }
        ],
        returns: 'file',
        example: 'f := open("data.txt", "r")\ncontent := f.read()\nf.close()'
    },
    'input': {
        name: 'input',
        signature: 'input([prompt])',
        description: 'Displays an optional prompt message and reads a single line of input from standard input.',
        params: [{ name: 'prompt', doc: 'Optional prompt text to display' }],
        returns: 'string',
        example: 'name := input("Enter your name: ")'
    },
    'hex': {
        name: 'hex',
        signature: 'hex(integer)',
        description: 'Converts an integer number to its hexadecimal string representation (e.g. "ff" or "0xff").',
        params: [{ name: 'integer', doc: 'Integer number to convert' }],
        returns: 'string',
        example: 's := hex(255) // "ff"'
    },
    'bin': {
        name: 'bin',
        signature: 'bin(integer)',
        description: 'Converts an integer number to its binary string representation (e.g. "101010").',
        params: [{ name: 'integer', doc: 'Integer number to convert' }],
        returns: 'string',
        example: 's := bin(42) // "101010"'
    },
    'oct': {
        name: 'oct',
        signature: 'oct(integer)',
        description: 'Converts an integer number to its octal string representation (e.g. "100").',
        params: [{ name: 'integer', doc: 'Integer number to convert' }],
        returns: 'string',
        example: 's := oct(64) // "100"'
    },
    'format': {
        name: 'format',
        signature: 'format(value, [format_spec])',
        description: 'Converts a value to a formatted representation as controlled by format_spec.',
        params: [
            { name: 'value', doc: 'Value to format' },
            { name: 'format_spec', doc: 'Optional format specification string' }
        ],
        returns: 'string',
        example: 's := format(123.456, ".2f")'
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
    },
    'split': {
        name: 'split',
        signature: 'split(str, delimiter)',
        description: 'Splits string `str` by `delimiter` into a dynamic list of substrings. If delimiter is empty, splits into characters.',
        params: [{ name: 'str', doc: 'Input string' }, { name: 'delimiter', doc: 'Separator delimiter string' }],
        returns: 'list',
        example: 'words := split("apple,banana,cherry", ",")\nprintln(words)'
    },
    'join': {
        name: 'join',
        signature: 'join(list, delimiter)',
        description: 'Joins all elements of a list into a single string separated by `delimiter`.',
        params: [{ name: 'list', doc: 'List of elements to join' }, { name: 'delimiter', doc: 'Separator delimiter string' }],
        returns: 'string',
        example: 's := join(["a", "b", "c"], "-") // "a-b-c"'
    },
    'upper': {
        name: 'upper',
        signature: 'upper(str)',
        description: 'Returns a new string with all ASCII characters converted to uppercase.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'string',
        example: 's := upper("hello") // "HELLO"'
    },
    'lower': {
        name: 'lower',
        signature: 'lower(str)',
        description: 'Returns a new string with all ASCII characters converted to lowercase.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'string',
        example: 's := lower("HELLO") // "hello"'
    },
    'trim': {
        name: 'trim',
        signature: 'trim(str)',
        description: 'Returns a new string with all leading and trailing whitespace characters removed.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'string',
        example: 's := trim("   hello   ") // "hello"'
    },
    'trimleft': {
        name: 'trimleft',
        signature: 'trimleft(str)',
        description: 'Returns a new string with all leading whitespace characters removed.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'string',
        example: 's := trimleft("   hello") // "hello"'
    },
    'trimright': {
        name: 'trimright',
        signature: 'trimright(str)',
        description: 'Returns a new string with all trailing whitespace characters removed.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'string',
        example: 's := trimright("hello   ") // "hello"'
    },
    'contains': {
        name: 'contains',
        signature: 'contains(str_or_list, target)',
        description: 'Returns true if `target` is found in the string or list, otherwise false.',
        params: [{ name: 'str_or_list', doc: 'String or list' }, { name: 'target', doc: 'Substring or item to search for' }],
        returns: 'bool',
        example: 'if contains("skylang", "sky") { println("yes") }'
    },
    'startswith': {
        name: 'startswith',
        signature: 'startswith(str, prefix)',
        description: 'Returns true if `str` begins with the specified `prefix`.',
        params: [{ name: 'str', doc: 'Input string' }, { name: 'prefix', doc: 'Prefix to check' }],
        returns: 'bool',
        example: 'if startswith("skylang", "sky") { println("yes") }'
    },
    'endswith': {
        name: 'endswith',
        signature: 'endswith(str, suffix)',
        description: 'Returns true if `str` ends with the specified `suffix`.',
        params: [{ name: 'str', doc: 'Input string' }, { name: 'suffix', doc: 'Suffix to check' }],
        returns: 'bool',
        example: 'if endswith("file.sky", ".sky") { println("yes") }'
    },
    'replace': {
        name: 'replace',
        signature: 'replace(str, old_sub, new_sub)',
        description: 'Returns a new string where all occurrences of `old_sub` are replaced by `new_sub`.',
        params: [{ name: 'str', doc: 'Input string' }, { name: 'old_sub', doc: 'Substring to replace' }, { name: 'new_sub', doc: 'Replacement substring' }],
        returns: 'string',
        example: 's := replace("banana", "a", "o") // "bonono"'
    },
    'find': {
        name: 'find',
        signature: 'find(str_or_list, target)',
        description: 'Returns the 0-based index of the first occurrence of `target` in the string or list, or -1 if not found.',
        params: [{ name: 'str_or_list', doc: 'String or list' }, { name: 'target', doc: 'Substring or item to search for' }],
        returns: 'int',
        example: 'idx := find("skylang", "lan") // 3'
    },
    'count': {
        name: 'count',
        signature: 'count(str_or_list, target)',
        description: 'Returns the number of non-overlapping occurrences of `target` in the string or list.',
        params: [{ name: 'str_or_list', doc: 'String or list' }, { name: 'target', doc: 'Substring or item to count' }],
        returns: 'int',
        example: 'n := count("banana", "a") // 3'
    },
    'reverse': {
        name: 'reverse',
        signature: 'reverse(str_or_list)',
        description: 'Returns a new string or list with elements in reverse order.',
        params: [{ name: 'str_or_list', doc: 'Input string or list' }],
        returns: 'string | list',
        example: 'r := reverse("skylang") // "gnalyks"'
    },
    'chars': {
        name: 'chars',
        signature: 'chars(str)',
        description: 'Returns a dynamic list containing each character of `str` as a string element.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'list',
        example: 'c := chars("Sky") // ["S", "k", "y"]'
    },
    'bytes': {
        name: 'bytes',
        signature: 'bytes(str)',
        description: 'Returns a dynamic list containing the integer ASCII byte values of `str`.',
        params: [{ name: 'str', doc: 'Input string' }],
        returns: 'list',
        example: 'b := bytes("ABC") // [65, 66, 67]'
    }
};

export const COLLECTION_METHODS: Record<string, BuiltinDoc> = {
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
        signature: 'dict.has(key) / set.has(item) / str.has(substr) / list.has(item)',
        description: 'Returns `true` if the key exists in the dictionary, item in the set/list, or substring in the string, otherwise `false`.',
        params: [{ name: 'key_or_item', doc: 'Key, item, or substring to test' }],
        returns: 'bool',
        example: 'if scores.has("Alice") {\n    println("Alice found!")\n}',
        container: 'dict | set | string | list'
    },
    'value': {
        name: 'value',
        signature: 'str.value(index)',
        description: 'Returns the single character at the specified 0-based index in the string.',
        params: [{ name: 'index', doc: 'Character index (0-based)' }],
        returns: 'char',
        example: 'ch := name.value(0) // \'S\'',
        container: 'string'
    },
    'split': {
        name: 'split',
        signature: 'str.split([delimiter])',
        description: 'Splits the string into a list of substrings separated by delimiter.',
        params: [{ name: 'delimiter', doc: 'Separator delimiter string (optional)' }],
        returns: 'list',
        example: 'words := "hello,world".split(",")',
        container: 'string'
    },
    'join': {
        name: 'join',
        signature: 'str.join(list) / list.join(delimiter)',
        description: 'Joins elements of a list with the string delimiter.',
        params: [{ name: 'arg', doc: 'List to join or string delimiter' }],
        returns: 'string',
        example: '", ".join(["a", "b", "c"])\n["a", "b"].join("-")',
        container: 'string | list'
    },
    'upper': {
        name: 'upper',
        signature: 'str.upper()',
        description: 'Returns uppercase version of the string.',
        returns: 'string',
        example: '"hello".upper() // "HELLO"',
        container: 'string'
    },
    'lower': {
        name: 'lower',
        signature: 'str.lower()',
        description: 'Returns lowercase version of the string.',
        returns: 'string',
        example: '"HELLO".lower() // "hello"',
        container: 'string'
    },
    'trim': {
        name: 'trim',
        signature: 'str.trim()',
        description: 'Returns string stripped of leading and trailing whitespace.',
        returns: 'string',
        example: '"  hello  ".trim() // "hello"',
        container: 'string'
    },
    'trimleft': {
        name: 'trimleft',
        signature: 'str.trimleft()',
        description: 'Returns string stripped of leading whitespace.',
        returns: 'string',
        example: '"  hello".trimleft() // "hello"',
        container: 'string'
    },
    'trimright': {
        name: 'trimright',
        signature: 'str.trimright()',
        description: 'Returns string stripped of trailing whitespace.',
        returns: 'string',
        example: '"hello  ".trimright() // "hello"',
        container: 'string'
    },
    'contains': {
        name: 'contains',
        signature: 'str.contains(substr) / list.contains(item)',
        description: 'Returns true if substring or item exists.',
        params: [{ name: 'target', doc: 'Substring or item to look for' }],
        returns: 'bool',
        example: '"skylang".contains("sky") // true',
        container: 'string | list'
    },
    'startswith': {
        name: 'startswith',
        signature: 'str.startswith(prefix)',
        description: 'Returns true if string starts with prefix.',
        params: [{ name: 'prefix', doc: 'Prefix string' }],
        returns: 'bool',
        example: '"skylang".startswith("sky") // true',
        container: 'string'
    },
    'endswith': {
        name: 'endswith',
        signature: 'str.endswith(suffix)',
        description: 'Returns true if string ends with suffix.',
        params: [{ name: 'suffix', doc: 'Suffix string' }],
        returns: 'bool',
        example: '"file.sky".endswith(".sky") // true',
        container: 'string'
    },
    'replace': {
        name: 'replace',
        signature: 'str.replace(old_sub, new_sub)',
        description: 'Returns new string with old_sub replaced by new_sub.',
        params: [{ name: 'old_sub', doc: 'Old substring' }, { name: 'new_sub', doc: 'New substring' }],
        returns: 'string',
        example: '"banana".replace("a", "o") // "bonono"',
        container: 'string'
    },
    'find': {
        name: 'find',
        signature: 'str.find(substr) / list.find(item)',
        description: 'Returns 0-based index of target, or -1 if not found.',
        params: [{ name: 'target', doc: 'Target to search for' }],
        returns: 'int',
        example: '"skylang".find("lan") // 3',
        container: 'string | list'
    },
    'count': {
        name: 'count',
        signature: 'str.count(substr) / list.count(item)',
        description: 'Returns count of occurrences.',
        params: [{ name: 'target', doc: 'Target to count' }],
        returns: 'int',
        example: '"banana".count("a") // 3',
        container: 'string | list'
    },
    'reverse': {
        name: 'reverse',
        signature: 'str.reverse() / list.reverse()',
        description: 'Returns reversed string or list.',
        returns: 'string | list',
        example: '"hello".reverse() // "olleh"',
        container: 'string | list'
    },
    'format': {
        name: 'format',
        signature: 'str.format(...args)',
        description: 'Formats the string template by replacing each `{}` placeholder with the string representations of the arguments.',
        params: [{ name: '...args', doc: 'Values to interpolate into {} placeholders' }],
        returns: 'string',
        example: 'msg := "User: {}, Score: {}".format("Alice", 95)',
        container: 'string'
    },
    'rjust': {
        name: 'rjust',
        signature: 'str.rjust(width, [fillchar])',
        description: 'Right-justifies the string in a field of given width, padded with spaces or fillchar.',
        params: [{ name: 'width', doc: 'Target total width' }, { name: 'fillchar', doc: 'Optional padding character (default space)' }],
        returns: 'string',
        example: '"42".rjust(5) // "   42"',
        container: 'string'
    },
    'ljust': {
        name: 'ljust',
        signature: 'str.ljust(width, [fillchar])',
        description: 'Left-justifies the string in a field of given width, padded with spaces or fillchar.',
        params: [{ name: 'width', doc: 'Target total width' }, { name: 'fillchar', doc: 'Optional padding character (default space)' }],
        returns: 'string',
        example: '"Sky".ljust(6) // "Sky   "',
        container: 'string'
    },
    'center': {
        name: 'center',
        signature: 'str.center(width, [fillchar])',
        description: 'Centers the string in a field of given width, padded with spaces or fillchar.',
        params: [{ name: 'width', doc: 'Target total width' }, { name: 'fillchar', doc: 'Optional padding character (default space)' }],
        returns: 'string',
        example: '"Sky".center(7) // "  Sky  "',
        container: 'string'
    },
    'zfill': {
        name: 'zfill',
        signature: 'str.zfill(width)',
        description: 'Pads the string with leading zeros until it reaches the specified width.',
        params: [{ name: 'width', doc: 'Target total character width' }],
        returns: 'string',
        example: '"42".zfill(5) // "00042"',
        container: 'string'
    },
    'read': {
        name: 'read',
        signature: 'file.read([size])',
        description: 'Reads at most size bytes/chars from the open file, or the entire file if size is omitted.',
        params: [{ name: 'size', doc: 'Optional byte/character count limit' }],
        returns: 'string',
        example: 'content := f.read()',
        container: 'file'
    },
    'write': {
        name: 'write',
        signature: 'file.write(content)',
        description: 'Writes string content to the open file.',
        params: [{ name: 'content', doc: 'String content to write' }],
        returns: 'int',
        example: 'f.write("Hello Skylang\\n")',
        container: 'file'
    },
    'readline': {
        name: 'readline',
        signature: 'file.readline()',
        description: 'Reads a single line from the open file stream.',
        returns: 'string',
        example: 'line := f.readline()',
        container: 'file'
    },
    'readlines': {
        name: 'readlines',
        signature: 'file.readlines()',
        description: 'Reads all remaining lines from the open file and returns them as a list of strings.',
        returns: 'list',
        example: 'lines := f.readlines()',
        container: 'file'
    },
    'close': {
        name: 'close',
        signature: 'file.close()',
        description: 'Flushes and closes the open file stream.',
        returns: 'none',
        example: 'f.close()',
        container: 'file'
    },
    'chars': {
        name: 'chars',
        signature: 'str.chars()',
        description: 'Returns dynamic list containing each character of the string.',
        returns: 'list',
        example: '"Sky".chars() // ["S", "k", "y"]',
        container: 'string'
    },
    'bytes': {
        name: 'bytes',
        signature: 'str.bytes()',
        description: 'Returns dynamic list containing integer ASCII byte values.',
        returns: 'list',
        example: '"ABC".bytes() // [65, 66, 67]',
        container: 'string'
    }
};

export const PROPERTIES: Record<string, BuiltinDoc> = {
    'size': {
        name: 'size',
        signature: 'container.size / string.size',
        description: 'O(1) property returning the number of elements in an array, list, tuple, dictionary, set, sortedList, or length of a string.',
        returns: 'int',
        example: 'println("List size:", tasks.size)\nprintln("String length:", name.size)\nprintln("Dict entries:", scores.size)'
    },
    'length': {
        name: 'length',
        signature: 'container.length / string.length',
        description: 'Alias for .size property returning element count or string character count.',
        returns: 'int',
        example: 'println("Length:", "hello".length)'
    },
    'chars': {
        name: 'chars',
        signature: 'string.chars',
        description: 'Property returning a dynamic list of individual character strings.',
        returns: 'list',
        example: 'for c in "hello".chars { println(c) }'
    },
    'bytes': {
        name: 'bytes',
        signature: 'string.bytes',
        description: 'Property returning a dynamic list of integer ASCII byte values.',
        returns: 'list',
        example: 'println("Bytes:", "hello".bytes)'
    },
    'T': {
        name: 'T',
        signature: 'val.T',
        description: 'Returns the runtime type descriptor of the variable (e.g. `<type int>`, `<type string>`, `<type list>`).',
        returns: 'type',
        example: 'if x.T == "int" {\n    println("x is an integer")\n}\nprintln("Type:", tasks.T)'
    }
};

export const CONTAINER_TYPE_MEMBERS: Record<string, { methods: string[]; properties: string[] }> = {
    'list': {
        methods: ['push', 'pop', 'clear', 'join', 'contains', 'has', 'find', 'count', 'reverse'],
        properties: ['size', 'length', 'T']
    },
    'sortedList': {
        methods: ['push', 'pop'],
        properties: ['size', 'length', 'T']
    },
    'dict': {
        methods: ['has'],
        properties: ['size', 'length', 'T']
    },
    'set': {
        methods: ['add', 'has'],
        properties: ['size', 'length', 'T']
    },
    'string': {
        methods: ['value', 'split', 'join', 'format', 'upper', 'lower', 'trim', 'trimleft', 'trimright', 'contains', 'has', 'startswith', 'endswith', 'replace', 'find', 'count', 'reverse', 'rjust', 'ljust', 'center', 'zfill', 'chars', 'bytes'],
        properties: ['size', 'length', 'chars', 'bytes', 'T']
    },
    'file': {
        methods: ['read', 'write', 'readline', 'readlines', 'close'],
        properties: ['T']
    },
    'array': {
        methods: [],
        properties: ['size', 'length', 'T']
    },
    'tuple': {
        methods: [],
        properties: ['size', 'length', 'T']
    }
};
