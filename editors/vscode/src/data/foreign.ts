import { BuiltinDoc } from './builtins';

export interface ForeignMethodDoc extends BuiltinDoc {
    snippet?: string;
}

export interface ForeignModuleDoc {
    name: string;
    bridge: 'js' | 'python' | 'java' | 'go' | 'rust' | 'cpp';
    description: string;
    methods: Record<string, ForeignMethodDoc>;
    properties?: Record<string, BuiltinDoc>;
}

export const FOREIGN_MODULE_ALIASES: Record<string, string> = {
    // JS aliases
    'express': 'express',
    'expressjs': 'express',
    'express.js': 'express',
    'express-js': 'express',
    'app': 'express',
    'server': 'express',
    'router': 'express',
    'lodash': 'lodash',
    '_': 'lodash',
    'axios': 'axios',
    'fs': 'fs',
    'fs-extra': 'fs',
    'node:fs': 'fs',
    'path': 'path',
    'node:path': 'path',
    'http': 'http',
    'node:http': 'http',
    'https': 'http',
    'crypto': 'crypto',
    'node:crypto': 'crypto',
    'math_js': 'Math',
    'Math': 'Math',
    'json_js': 'JSON',
    'JSON': 'JSON',
    'ws': 'ws',
    'dotenv': 'dotenv',
    'chalk': 'chalk',
    'moment': 'moment',
    'dayjs': 'dayjs',
    'zod': 'zod',
    'cors': 'cors',
    'jwt': 'jsonwebtoken',
    'jsonwebtoken': 'jsonwebtoken',

    // Python aliases
    'numpy': 'numpy',
    'np': 'numpy',
    'pandas': 'pandas',
    'pd': 'pandas',
    'torch': 'torch',
    'pytorch': 'torch',
    'scipy': 'scipy',
    'matplotlib': 'matplotlib',
    'matplotlib.pyplot': 'matplotlib',
    'plt': 'matplotlib',
    'requests': 'requests',
    'os': 'os',
    'sys': 'sys',
    'random': 'random',
    'json': 'json',
    're': 're',
    'datetime': 'datetime',
    'flask': 'flask',
    'fastapi': 'fastapi',

    // Java aliases
    'java.lang.Math': 'java.lang.Math',
    'java_math': 'java.lang.Math',
    'java.util.ArrayList': 'java.util.ArrayList',
    'ArrayList': 'java.util.ArrayList',
    'java.util.HashMap': 'java.util.HashMap',
    'HashMap': 'java.util.HashMap',
    'java.lang.System': 'java.lang.System',
    'System': 'java.lang.System',
    'java.util.Arrays': 'java.util.Arrays',
    'Arrays': 'java.util.Arrays',
    'java.util.Collections': 'java.util.Collections',
    'Collections': 'java.util.Collections',

    // Go aliases
    'fmt': 'fmt',
    'go_fmt': 'fmt',
    'strings': 'strings',
    'strconv': 'strconv',
    'time': 'time',
    'encoding/json': 'encoding/json',
    'net/http': 'net/http',

    // Rust aliases
    'serde': 'serde',
    'serde_json': 'serde_json',
    'tokio': 'tokio',
    'std::collections': 'std::collections',
    'std::fs': 'std::fs',
    'std::time': 'std::time'
};

export const FOREIGN_MODULES: Record<string, ForeignModuleDoc> = {
    // ==========================================
    // JS / Node.js Modules
    // ==========================================
    'express': {
        name: 'express (Express.js Web Framework)',
        bridge: 'js',
        description: 'Fast, unopinionated, minimalist web framework for Node.js loaded via `js.load("expressjs")` or `js.load("express")`.',
        methods: {
            'get': {
                name: 'get',
                signature: 'express.get(path, callback)',
                snippet: "get('${1:/}', (req, res) => {\n\t${0:res.send(\"Hello World\")}\n})",
                description: 'Routes HTTP GET requests to the specified path with the specified callback handler function.',
                params: [
                    { name: 'path', doc: 'URL route path string or pattern (e.g. "/", "/api/users")' },
                    { name: 'callback', doc: 'Request handler callback `(req, res) => { ... }`' }
                ],
                returns: 'app',
                example: 'app.get("/users", (req, res) => {\n    res.json({ users: ["Alice", "Bob"] })\n})'
            },
            'post': {
                name: 'post',
                signature: 'express.post(path, callback)',
                snippet: "post('${1:/}', (req, res) => {\n\t${0:res.json({ status: \"success\" })}\n})",
                description: 'Routes HTTP POST requests to the specified path with the specified callback handler function.',
                params: [
                    { name: 'path', doc: 'URL route path string or pattern' },
                    { name: 'callback', doc: 'Request handler callback `(req, res) => { ... }`' }
                ],
                returns: 'app',
                example: 'app.post("/api/login", (req, res) => {\n    res.json({ token: \"xyz\", status: \"ok\" })\n})'
            },
            'put': {
                name: 'put',
                signature: 'express.put(path, callback)',
                snippet: "put('${1:/:id}', (req, res) => {\n\t${0:res.json({ updated: true })}\n})",
                description: 'Routes HTTP PUT requests to the specified path.',
                params: [
                    { name: 'path', doc: 'URL route path string or pattern' },
                    { name: 'callback', doc: 'Request handler callback `(req, res) => { ... }`' }
                ],
                returns: 'app',
                example: 'app.put("/users/:id", (req, res) => {\n    res.json({ updated: true })\n})'
            },
            'delete': {
                name: 'delete',
                signature: 'express.delete(path, callback)',
                snippet: "delete('${1:/:id}', (req, res) => {\n\t${0:res.json({ deleted: true })}\n})",
                description: 'Routes HTTP DELETE requests to the specified path.',
                params: [
                    { name: 'path', doc: 'URL route path string or pattern' },
                    { name: 'callback', doc: 'Request handler callback `(req, res) => { ... }`' }
                ],
                returns: 'app',
                example: 'app.delete("/users/:id", (req, res) => {\n    res.json({ deleted: true })\n})'
            },
            'patch': {
                name: 'patch',
                signature: 'express.patch(path, callback)',
                snippet: "patch('${1:/:id}', (req, res) => {\n\t${0:res.json({ patched: true })}\n})",
                description: 'Routes HTTP PATCH requests to the specified path.',
                params: [
                    { name: 'path', doc: 'URL route path string or pattern' },
                    { name: 'callback', doc: 'Request handler callback `(req, res) => { ... }`' }
                ],
                returns: 'app',
                example: 'app.patch("/users/:id", (req, res) => {\n    res.json({ patched: true })\n})'
            },
            'use': {
                name: 'use',
                signature: 'express.use([path], middleware)',
                snippet: 'use(${1:express.json()})',
                description: 'Mounts the specified middleware function(s) at the specified path (defaults to "/").',
                params: [
                    { name: 'path', doc: 'Optional route path prefix to mount middleware on (default: "/")' },
                    { name: 'middleware', doc: 'Middleware function or router handler' }
                ],
                returns: 'app',
                example: 'app.use(express.json())\napp.use("/public", express.static("public"))'
            },
            'listen': {
                name: 'listen',
                signature: 'express.listen(port, [callback])',
                snippet: 'listen(${1:3000}, () => {\n\tprintln("Server listening on http://localhost:${1:3000}")\n\t${0}\n})',
                description: 'Binds and listens for connections on the specified port and host.',
                params: [
                    { name: 'port', doc: 'Port number to listen on (e.g. 3000, 8080)' },
                    { name: 'callback', doc: 'Optional callback executed when the server begins listening' }
                ],
                returns: 'http.Server',
                example: 'app.listen(3000, () => {\n    println("Express server running on port 3000")\n})'
            },
            'json': {
                name: 'json',
                signature: 'express.json([options])',
                snippet: 'json()',
                description: 'Built-in Express middleware that parses incoming request payloads with JSON format.',
                params: [{ name: 'options', doc: 'Optional configuration options object (e.g. { limit: "10mb" })' }],
                returns: 'middleware',
                example: 'app.use(express.json())'
            },
            'urlencoded': {
                name: 'urlencoded',
                signature: 'express.urlencoded([options])',
                snippet: 'urlencoded({ extended: ${1:true} })',
                description: 'Built-in Express middleware that parses incoming request payloads with URL-encoded data.',
                params: [{ name: 'options', doc: 'Configuration options with `extended: true | false`' }],
                returns: 'middleware',
                example: 'app.use(express.urlencoded({ extended: true }))'
            },
            'static': {
                name: 'static',
                signature: 'express.static(root, [options])',
                snippet: "static('${1:public}')",
                description: 'Built-in Express middleware that serves static files from the specified root directory.',
                params: [
                    { name: 'root', doc: 'Root directory containing static assets' },
                    { name: 'options', doc: 'Optional static file options' }
                ],
                returns: 'middleware',
                example: 'app.use(express.static("public"))'
            },
            'Router': {
                name: 'Router',
                signature: 'express.Router([options])',
                snippet: 'Router()',
                description: 'Creates a new modular, isolated Express Router instance.',
                params: [{ name: 'options', doc: 'Optional router options ({ caseSensitive: bool, mergeParams: bool })' }],
                returns: 'Router',
                example: 'router := express.Router()\nrouter.get("/status", (req, res) => res.json({ status: "online" }))\napp.use("/api", router)'
            },
            'set': {
                name: 'set',
                signature: 'express.set(setting, value)',
                snippet: "set('${1:view engine}', '${2:ejs}')",
                description: 'Assigns setting name to value in Express application configuration.',
                params: [{ name: 'setting', doc: 'Configuration setting name string' }, { name: 'value', doc: 'Setting value' }],
                returns: 'app',
                example: 'app.set("view engine", "ejs")'
            },
            'all': {
                name: 'all',
                signature: 'express.all(path, callback)',
                snippet: "all('${1:*}', (req, res) => {\n\t${0}\n})",
                description: 'Matches all HTTP methods (GET, POST, PUT, DELETE, etc.) on the specified path.',
                params: [{ name: 'path', doc: 'URL path pattern' }, { name: 'callback', doc: 'Handler callback' }],
                returns: 'app'
            },
            'route': {
                name: 'route',
                signature: 'express.route(path)',
                snippet: "route('${1:/api/users}')",
                description: 'Returns an instance of a single route for chaining multiple HTTP verbs.',
                params: [{ name: 'path', doc: 'URL path pattern' }],
                returns: 'Route',
                example: 'app.route("/book")\n    .get((req, res) => res.send("Get book"))\n    .post((req, res) => res.send("Add book"))'
            },
            'param': {
                name: 'param',
                signature: 'express.param(name, callback)',
                snippet: "param('${1:id}', (req, res, next, id) => {\n\t${0}\n\tnext()\n})",
                description: 'Adds callback triggers to route parameters.',
                params: [{ name: 'name', doc: 'Parameter name string' }, { name: 'callback', doc: 'Callback function `(req, res, next, id) => ...`' }]
            },
            'engine': {
                name: 'engine',
                signature: 'express.engine(ext, callback)',
                snippet: "engine('${1:html}', ${2:callback})",
                description: 'Registers the given template engine callback for mapping file extension.',
                params: [{ name: 'ext', doc: 'File extension (e.g. "html", "pug")' }, { name: 'callback', doc: 'Template render engine function' }]
            },
            'disable': {
                name: 'disable',
                signature: 'express.disable(setting)',
                snippet: "disable('${1:x-powered-by}')",
                description: 'Sets Boolean setting to false.',
                params: [{ name: 'setting', doc: 'Setting name string' }]
            },
            'enable': {
                name: 'enable',
                signature: 'express.enable(setting)',
                snippet: "enable('${1:trust proxy}')",
                description: 'Sets Boolean setting to true.',
                params: [{ name: 'setting', doc: 'Setting name string' }]
            },
            'raw': {
                name: 'raw',
                signature: 'express.raw([options])',
                snippet: 'raw()',
                description: 'Built-in Express middleware that parses incoming request payloads into a Buffer.',
                params: [{ name: 'options', doc: 'Parser options' }],
                returns: 'middleware'
            },
            'text': {
                name: 'text',
                signature: 'express.text([options])',
                snippet: 'text()',
                description: 'Built-in Express middleware that parses incoming request payloads into a string.',
                params: [{ name: 'options', doc: 'Parser options' }],
                returns: 'middleware'
            }
        },
        properties: {
            'application': { name: 'application', signature: 'express.application', description: 'Express application prototype object', returns: 'object' },
            'request': { name: 'request', signature: 'express.request', description: 'Express HTTP request prototype object', returns: 'object' },
            'response': { name: 'response', signature: 'express.response', description: 'Express HTTP response prototype object', returns: 'object' }
        }
    },

    'lodash': {
        name: 'lodash (JavaScript Utility Library)',
        bridge: 'js',
        description: 'A modern JavaScript utility library delivering modularity, performance & extras.',
        methods: {
            'map': { name: 'map', signature: '_.map(collection, iteratee)', snippet: 'map(${1:collection}, ${2:item => item})', description: 'Creates an array of values by running each element in collection through iteratee.', returns: 'array' },
            'filter': { name: 'filter', signature: '_.filter(collection, predicate)', snippet: 'filter(${1:collection}, ${2:item => true})', description: 'Iterates over elements of collection, returning an array of all elements predicate returns truthy for.', returns: 'array' },
            'reduce': { name: 'reduce', signature: '_.reduce(collection, iteratee, [accumulator])', snippet: 'reduce(${1:collection}, ${(acc, item) => acc}, ${2:0})', description: 'Reduces collection to a value which is the accumulated result of running each element through iteratee.', returns: 'any' },
            'find': { name: 'find', signature: '_.find(collection, predicate)', snippet: 'find(${1:collection}, ${2:item => true})', description: 'Iterates over elements of collection, returning the first element predicate returns truthy for.', returns: 'any' },
            'chunk': { name: 'chunk', signature: '_.chunk(array, [size=1])', snippet: 'chunk(${1:array}, ${2:2})', description: 'Creates an array of elements split into groups the length of size.', returns: 'array' },
            'cloneDeep': { name: 'cloneDeep', signature: '_.cloneDeep(value)', snippet: 'cloneDeep(${1:value})', description: 'Creates a deep clone of value.', returns: 'any' },
            'merge': { name: 'merge', signature: '_.merge(object, [sources])', snippet: 'merge(${1:target}, ${2:source})', description: 'Recursively merges own and inherited enumerable string keyed properties of source objects into target object.', returns: 'object' },
            'groupBy': { name: 'groupBy', signature: '_.groupBy(collection, iteratee)', snippet: 'groupBy(${1:collection}, ${2:key})', description: 'Creates an object composed of keys generated from the results of running each element of collection through iteratee.', returns: 'object' },
            'uniq': { name: 'uniq', signature: '_.uniq(array)', snippet: 'uniq(${1:array})', description: 'Creates a duplicate-free version of an array.', returns: 'array' },
            'sortBy': { name: 'sortBy', signature: '_.sortBy(collection, [iteratees])', snippet: 'sortBy(${1:collection}, [${2:key}])', description: 'Creates an array of elements, sorted in ascending order by the results of running each element through each iteratee.', returns: 'array' },
            'debounce': { name: 'debounce', signature: '_.debounce(func, [wait=0], [options={}])', snippet: 'debounce(${1:func}, ${2:300})', description: 'Creates a debounced function that delays invoking func until after wait milliseconds have elapsed.', returns: 'function' },
            'throttle': { name: 'throttle', signature: '_.throttle(func, [wait=0], [options={}])', snippet: 'throttle(${1:func}, ${2:300})', description: 'Creates a throttled function that only invokes func at most once per every wait milliseconds.', returns: 'function' },
            'isEmpty': { name: 'isEmpty', signature: '_.isEmpty(value)', snippet: 'isEmpty(${1:value})', description: 'Checks if value is an empty object, collection, map, or set.', returns: 'bool' },
            'isEqual': { name: 'isEqual', signature: '_.isEqual(value, other)', snippet: 'isEqual(${1:a}, ${2:b})', description: 'Performs a deep comparison between two values to determine if they are equivalent.', returns: 'bool' },
            'get': { name: 'get', signature: '_.get(object, path, [defaultValue])', snippet: "get(${1:object}, '${2:path}', ${3:default})", description: 'Gets the value at path of object. If resolved value is undefined, defaultValue is returned.', returns: 'any' },
            'set': { name: 'set', signature: '_.set(object, path, value)', snippet: "set(${1:object}, '${2:path}', ${3:value})", description: 'Sets the value at path of object.', returns: 'object' }
        }
    },

    'axios': {
        name: 'axios (Promise-based HTTP Client)',
        bridge: 'js',
        description: 'Promise based HTTP client for the browser and Node.js.',
        methods: {
            'get': { name: 'get', signature: 'axios.get(url, [config])', snippet: "get('${1:https://api.example.com/data}')", description: 'Sends an HTTP GET request to the specified URL.', returns: 'Promise' },
            'post': { name: 'post', signature: 'axios.post(url, [data], [config])', snippet: "post('${1:https://api.example.com/data}', ${2:payload})", description: 'Sends an HTTP POST request with payload to the specified URL.', returns: 'Promise' },
            'put': { name: 'put', signature: 'axios.put(url, [data], [config])', snippet: "put('${1:https://api.example.com/data/:id}', ${2:payload})", description: 'Sends an HTTP PUT request.', returns: 'Promise' },
            'delete': { name: 'delete', signature: 'axios.delete(url, [config])', snippet: "delete('${1:https://api.example.com/data/:id}')", description: 'Sends an HTTP DELETE request.', returns: 'Promise' },
            'patch': { name: 'patch', signature: 'axios.patch(url, [data], [config])', snippet: "patch('${1:https://api.example.com/data/:id}', ${2:payload})", description: 'Sends an HTTP PATCH request.', returns: 'Promise' },
            'request': { name: 'request', signature: 'axios.request(config)', snippet: 'request(${1:config})', description: 'Sends a custom configured HTTP request.', returns: 'Promise' },
            'create': { name: 'create', signature: 'axios.create([config])', snippet: 'create({ baseURL: "${1:https://api.example.com}" })', description: 'Creates a new Axios instance with custom default configuration.', returns: 'AxiosInstance' }
        }
    },

    'fs': {
        name: 'fs (Node.js File System Module)',
        bridge: 'js',
        description: 'Node.js built-in file system module providing synchronous and asynchronous file I/O operations.',
        methods: {
            'readFileSync': { name: 'readFileSync', signature: 'fs.readFileSync(path, [options])', snippet: "readFileSync('${1:file.txt}', 'utf8')", description: 'Synchronously reads the entire contents of a file.', returns: 'string | Buffer' },
            'writeFileSync': { name: 'writeFileSync', signature: 'fs.writeFileSync(file, data, [options])', snippet: "writeFileSync('${1:file.txt}', ${2:data}, 'utf8')", description: 'Synchronously writes data to a file.', returns: 'none' },
            'existsSync': { name: 'existsSync', signature: 'fs.existsSync(path)', snippet: "existsSync('${1:path}')", description: 'Synchronously tests whether the specified path exists.', returns: 'bool' },
            'mkdirSync': { name: 'mkdirSync', signature: 'fs.mkdirSync(path, [options])', snippet: "mkdirSync('${1:dir}', { recursive: true })", description: 'Synchronously creates a directory.', returns: 'string' },
            'readdirSync': { name: 'readdirSync', signature: 'fs.readdirSync(path, [options])', snippet: "readdirSync('${1:dir}')", description: 'Synchronously reads the contents of a directory.', returns: 'array' },
            'statSync': { name: 'statSync', signature: 'fs.statSync(path)', snippet: "statSync('${1:path}')", description: 'Synchronously retrieves file status information.', returns: 'Stats' },
            'unlinkSync': { name: 'unlinkSync', signature: 'fs.unlinkSync(path)', snippet: "unlinkSync('${1:file.txt}')", description: 'Synchronously removes a file.', returns: 'none' },
            'copyFileSync': { name: 'copyFileSync', signature: 'fs.copyFileSync(src, dest)', snippet: "copyFileSync('${1:src}', '${2:dest}')", description: 'Synchronously copies src to dest.', returns: 'none' }
        }
    },

    'path': {
        name: 'path (Node.js Path Utility Module)',
        bridge: 'js',
        description: 'Node.js built-in path utilities for working with file and directory paths.',
        methods: {
            'join': { name: 'join', signature: 'path.join(...paths)', snippet: "join(${1:__dirname}, '${2:sub}')", description: 'Joins all given path segments together using the platform-specific delimiter.', returns: 'string' },
            'resolve': { name: 'resolve', signature: 'path.resolve(...paths)', snippet: "resolve('${1:./path}')", description: 'Resolves a sequence of paths or path segments into an absolute path.', returns: 'string' },
            'basename': { name: 'basename', signature: 'path.basename(path, [ext])', snippet: "basename('${1:path}')", description: 'Returns the last portion of a path.', returns: 'string' },
            'dirname': { name: 'dirname', signature: 'path.dirname(path)', snippet: "dirname('${1:path}')", description: 'Returns the directory name of a path.', returns: 'string' },
            'extname': { name: 'extname', signature: 'path.extname(path)', snippet: "extname('${1:file.txt}')", description: 'Returns the extension of the path.', returns: 'string' },
            'isAbsolute': { name: 'isAbsolute', signature: 'path.isAbsolute(path)', snippet: "isAbsolute('${1:path}')", description: 'Determines whether path is an absolute path.', returns: 'bool' }
        }
    },

    'Math': {
        name: 'Math (JavaScript Math Global)',
        bridge: 'js',
        description: 'Standard JavaScript Math global object providing mathematical constants and functions.',
        methods: {
            'sqrt': { name: 'sqrt', signature: 'Math.sqrt(x)', snippet: 'sqrt(${1:x})', description: 'Returns the square root of a number.', returns: 'double' },
            'pow': { name: 'pow', signature: 'Math.pow(base, exponent)', snippet: 'pow(${1:base}, ${2:exp})', description: 'Returns base raised to exponent power.', returns: 'double' },
            'max': { name: 'max', signature: 'Math.max(...values)', snippet: 'max(${1:a}, ${2:b})', description: 'Returns the largest of the given numbers.', returns: 'double' },
            'min': { name: 'min', signature: 'Math.min(...values)', snippet: 'min(${1:a}, ${2:b})', description: 'Returns the smallest of the given numbers.', returns: 'double' },
            'floor': { name: 'floor', signature: 'Math.floor(x)', snippet: 'floor(${1:x})', description: 'Returns the largest integer less than or equal to x.', returns: 'int' },
            'ceil': { name: 'ceil', signature: 'Math.ceil(x)', snippet: 'ceil(${1:x})', description: 'Returns the smallest integer greater than or equal to x.', returns: 'int' },
            'round': { name: 'round', signature: 'Math.round(x)', snippet: 'round(${1:x})', description: 'Returns value rounded to nearest integer.', returns: 'int' },
            'abs': { name: 'abs', signature: 'Math.abs(x)', snippet: 'abs(${1:x})', description: 'Returns absolute value.', returns: 'double' },
            'random': { name: 'random', signature: 'Math.random()', snippet: 'random()', description: 'Returns pseudo-random floating point number in range [0, 1).', returns: 'double' }
        },
        properties: {
            'PI': { name: 'PI', signature: 'Math.PI', description: 'Mathematical constant π (~3.14159)', returns: 'double' },
            'E': { name: 'E', signature: 'Math.E', description: 'Euler\'s constant e (~2.71828)', returns: 'double' }
        }
    },

    'JSON': {
        name: 'JSON (JavaScript JSON Global)',
        bridge: 'js',
        description: 'Standard JavaScript JSON global object for parsing and stringifying JSON data.',
        methods: {
            'stringify': { name: 'stringify', signature: 'JSON.stringify(value, [replacer], [space])', snippet: 'stringify(${1:value}, null, 2)', description: 'Converts a JavaScript value to a JSON string.', returns: 'string' },
            'parse': { name: 'parse', signature: 'JSON.parse(text, [reviver])', snippet: 'parse(${1:jsonStr})', description: 'Parses a JSON string into a JavaScript value or object.', returns: 'any' }
        }
    },

    // ==========================================
    // Python Modules
    // ==========================================
    'numpy': {
        name: 'numpy (Python NumPy Library)',
        bridge: 'python',
        description: 'The fundamental package for scientific computing with Python.',
        methods: {
            'array': { name: 'array', signature: 'np.array(object, [dtype])', snippet: 'array([${1:1, 2, 3}])', description: 'Creates an N-dimensional array.', returns: 'ndarray' },
            'zeros': { name: 'zeros', signature: 'np.zeros(shape, [dtype])', snippet: 'zeros(${1:[3, 3]})', description: 'Returns a new array of given shape and type, filled with zeros.', returns: 'ndarray' },
            'ones': { name: 'ones', signature: 'np.ones(shape, [dtype])', snippet: 'ones(${1:[3, 3]})', description: 'Returns a new array of given shape and type, filled with ones.', returns: 'ndarray' },
            'arange': { name: 'arange', signature: 'np.arange([start], stop, [step])', snippet: 'arange(${1:10})', description: 'Returns evenly spaced values within a given interval.', returns: 'ndarray' },
            'linspace': { name: 'linspace', signature: 'np.linspace(start, stop, num=50)', snippet: 'linspace(${1:0}, ${2:1}, ${3:100})', description: 'Returns evenly spaced numbers over a specified interval.', returns: 'ndarray' },
            'dot': { name: 'dot', signature: 'np.dot(a, b)', snippet: 'dot(${1:a}, ${2:b})', description: 'Dot product of two arrays.', returns: 'ndarray' },
            'matmul': { name: 'matmul', signature: 'np.matmul(x1, x2)', snippet: 'matmul(${1:a}, ${2:b})', description: 'Matrix product of two arrays.', returns: 'ndarray' },
            'sum': { name: 'sum', signature: 'np.sum(a, [axis])', snippet: 'sum(${1:a})', description: 'Sum of array elements over a given axis.', returns: 'number | ndarray' },
            'mean': { name: 'mean', signature: 'np.mean(a, [axis])', snippet: 'mean(${1:a})', description: 'Compute arithmetic mean along specified axis.', returns: 'number | ndarray' },
            'reshape': { name: 'reshape', signature: 'np.reshape(a, newshape)', snippet: 'reshape(${1:a}, ${2:[-1, 1]})', description: 'Gives a new shape to an array without changing its data.', returns: 'ndarray' }
        }
    },

    'pandas': {
        name: 'pandas (Python Data Analysis Library)',
        bridge: 'python',
        description: 'Powerful Python data structures and data analysis toolkit.',
        methods: {
            'DataFrame': { name: 'DataFrame', signature: 'pd.DataFrame(data, [index], [columns])', snippet: 'DataFrame(${1:data})', description: 'Two-dimensional, size-mutable, potentially heterogeneous tabular data.', returns: 'DataFrame' },
            'Series': { name: 'Series', signature: 'pd.Series(data, [index])', snippet: 'Series(${1:data})', description: 'One-dimensional ndarray with axis labels.', returns: 'Series' },
            'read_csv': { name: 'read_csv', signature: 'pd.read_csv(filepath_or_buffer)', snippet: "read_csv('${1:data.csv}')", description: 'Read a comma-separated values (csv) file into DataFrame.', returns: 'DataFrame' },
            'read_json': { name: 'read_json', signature: 'pd.read_json(path_or_buf)', snippet: "read_json('${1:data.json}')", description: 'Convert a JSON string to pandas object.', returns: 'DataFrame' },
            'concat': { name: 'concat', signature: 'pd.concat(objs, [axis])', snippet: 'concat([${1:df1, df2}])', description: 'Concatenate pandas objects along a particular axis.', returns: 'DataFrame' }
        }
    },

    'torch': {
        name: 'torch (PyTorch Deep Learning Framework)',
        bridge: 'python',
        description: 'Tensors and Dynamic neural networks in Python with strong GPU acceleration.',
        methods: {
            'tensor': { name: 'tensor', signature: 'torch.tensor(data, [dtype], [device])', snippet: 'tensor([${1:1.0, 2.0, 3.0}])', description: 'Constructs a tensor with data.', returns: 'Tensor' },
            'zeros': { name: 'zeros', signature: 'torch.zeros(*size)', snippet: 'zeros(${1:3, 3})', description: 'Returns a tensor filled with scalar value 0.', returns: 'Tensor' },
            'ones': { name: 'ones', signature: 'torch.ones(*size)', snippet: 'ones(${1:3, 3})', description: 'Returns a tensor filled with scalar value 1.', returns: 'Tensor' },
            'randn': { name: 'randn', signature: 'torch.randn(*size)', snippet: 'randn(${1:3, 3})', description: 'Returns a tensor filled with random numbers from normal distribution.', returns: 'Tensor' },
            'matmul': { name: 'matmul', signature: 'torch.matmul(input, other)', snippet: 'matmul(${1:tensor1}, ${2:tensor2})', description: 'Matrix product of two tensors.', returns: 'Tensor' }
        }
    },

    'matplotlib': {
        name: 'matplotlib.pyplot (Python Plotting Library)',
        bridge: 'python',
        description: 'Comprehensive 2D plotting library for Python.',
        methods: {
            'plot': { name: 'plot', signature: 'plt.plot(*args, **kwargs)', snippet: 'plot(${1:x}, ${2:y})', description: 'Plot y versus x as lines and/or markers.', returns: 'list' },
            'scatter': { name: 'scatter', signature: 'plt.scatter(x, y)', snippet: 'scatter(${1:x}, ${2:y})', description: 'A scatter plot of y vs. x.', returns: 'PathCollection' },
            'bar': { name: 'bar', signature: 'plt.bar(x, height)', snippet: 'bar(${1:categories}, ${2:values})', description: 'Make a bar plot.', returns: 'BarContainer' },
            'show': { name: 'show', signature: 'plt.show()', snippet: 'show()', description: 'Display all open figures.', returns: 'none' },
            'savefig': { name: 'savefig', signature: 'plt.savefig(fname)', snippet: "savefig('${1:plot.png}')", description: 'Save current figure to disk.', returns: 'none' },
            'title': { name: 'title', signature: 'plt.title(label)', snippet: "title('${1:Title}')", description: 'Set a title for the axes.', returns: 'Text' },
            'xlabel': { name: 'xlabel', signature: 'plt.xlabel(xlabel)', snippet: "xlabel('${1:X Axis}')", description: 'Set the label for the x-axis.', returns: 'Text' },
            'ylabel': { name: 'ylabel', signature: 'plt.ylabel(ylabel)', snippet: "ylabel('${1:Y Axis}')", description: 'Set the label for the y-axis.', returns: 'Text' }
        }
    },

    'requests': {
        name: 'requests (Python HTTP for Humans)',
        bridge: 'python',
        description: 'Simple and elegant HTTP library for Python.',
        methods: {
            'get': { name: 'get', signature: 'requests.get(url, [params], [headers])', snippet: "get('${1:https://api.example.com}')", description: 'Sends a GET request.', returns: 'Response' },
            'post': { name: 'post', signature: 'requests.post(url, [data], [json], [headers])', snippet: "post('${1:https://api.example.com}', json=${2:payload})", description: 'Sends a POST request.', returns: 'Response' },
            'put': { name: 'put', signature: 'requests.put(url, [data])', snippet: "put('${1:https://api.example.com}', json=${2:payload})", description: 'Sends a PUT request.', returns: 'Response' },
            'delete': { name: 'delete', signature: 'requests.delete(url)', snippet: "delete('${1:https://api.example.com}')", description: 'Sends a DELETE request.', returns: 'Response' }
        }
    },

    // ==========================================
    // Java Classes
    // ==========================================
    'java.lang.Math': {
        name: 'java.lang.Math (Java Math Class)',
        bridge: 'java',
        description: 'Java class containing methods for performing basic numeric operations.',
        methods: {
            'sqrt': { name: 'sqrt', signature: 'Math.sqrt(double a)', snippet: 'sqrt(${1:a})', description: 'Returns the correctly rounded positive square root of a double value.', returns: 'double' },
            'pow': { name: 'pow', signature: 'Math.pow(double a, double b)', snippet: 'pow(${1:a}, ${2:b})', description: 'Returns the value of the first argument raised to the power of the second argument.', returns: 'double' },
            'max': { name: 'max', signature: 'Math.max(a, b)', snippet: 'max(${1:a}, ${2:b})', description: 'Returns the greater of two values.', returns: 'number' },
            'min': { name: 'min', signature: 'Math.min(a, b)', snippet: 'min(${1:a}, ${2:b})', description: 'Returns the smaller of two values.', returns: 'number' },
            'abs': { name: 'abs', signature: 'Math.abs(a)', snippet: 'abs(${1:a})', description: 'Returns the absolute value.', returns: 'number' }
        },
        properties: {
            'PI': { name: 'PI', signature: 'Math.PI', description: 'The double value that is closer than any other to pi.', returns: 'double' },
            'E': { name: 'E', signature: 'Math.E', description: 'The double value that is closer than any other to e.', returns: 'double' }
        }
    },

    'java.util.ArrayList': {
        name: 'java.util.ArrayList (Java ArrayList Class)',
        bridge: 'java',
        description: 'Resizable-array implementation of the Java List interface.',
        methods: {
            'add': { name: 'add', signature: 'list.add(element)', snippet: 'add(${1:element})', description: 'Appends the specified element to the end of this list.', returns: 'boolean' },
            'get': { name: 'get', signature: 'list.get(index)', snippet: 'get(${1:index})', description: 'Returns the element at the specified position in this list.', returns: 'object' },
            'remove': { name: 'remove', signature: 'list.remove(index_or_object)', snippet: 'remove(${1:index})', description: 'Removes the element at the specified position in this list.', returns: 'object' },
            'size': { name: 'size', signature: 'list.size()', snippet: 'size()', description: 'Returns the number of elements in this list.', returns: 'int' },
            'clear': { name: 'clear', signature: 'list.clear()', snippet: 'clear()', description: 'Removes all of the elements from this list.', returns: 'none' }
        }
    },

    // ==========================================
    // Go Packages
    // ==========================================
    'fmt': {
        name: 'fmt (Go I/O Formatting Package)',
        bridge: 'go',
        description: 'Go package implementing formatted I/O with functions analogous to C printf and scanf.',
        methods: {
            'Println': { name: 'Println', signature: 'fmt.Println(a ...interface{})', snippet: 'Println(${1:a})', description: 'Formats using default formats and writes to standard output with newline.', returns: 'int, error' },
            'Printf': { name: 'Printf', signature: 'fmt.Printf(format string, a ...interface{})', snippet: "Printf('${1:%v}\\n', ${2:a})", description: 'Formats according to a format specifier and writes to standard output.', returns: 'int, error' },
            'Sprintf': { name: 'Sprintf', signature: 'fmt.Sprintf(format string, a ...interface{})', snippet: "Sprintf('${1:%v}', ${2:a})", description: 'Formats according to a format specifier and returns the resulting string.', returns: 'string' }
        }
    },

    // ==========================================
    // Rust Crates
    // ==========================================
    'serde_json': {
        name: 'serde_json (Rust JSON Serialization Crate)',
        bridge: 'rust',
        description: 'A JSON serialization file format library for Rust.',
        methods: {
            'to_string': { name: 'to_string', signature: 'serde_json::to_string(&value)', snippet: 'to_string(&${1:value})', description: 'Serialize the given data structure as a String of JSON.', returns: 'Result<String>' },
            'from_str': { name: 'from_str', signature: 'serde_json::from_str(&str)', snippet: 'from_str(&${1:json_str})', description: 'Deserialize an instance of type T from a string of JSON text.', returns: 'Result<T>' }
        }
    }
};

export function normalizeModuleName(rawName: string): string {
    const clean = rawName.trim().replace(/^["']|["']$/g, '');
    const lower = clean.toLowerCase();
    return FOREIGN_MODULE_ALIASES[clean] || FOREIGN_MODULE_ALIASES[lower] || clean;
}

export function getForeignModule(moduleName: string, bridge?: string): ForeignModuleDoc | undefined {
    const norm = normalizeModuleName(moduleName);
    if (FOREIGN_MODULES[norm]) {
        return FOREIGN_MODULES[norm];
    }
    if (FOREIGN_MODULES[norm.toLowerCase()]) {
        return FOREIGN_MODULES[norm.toLowerCase()];
    }
    return undefined;
}
