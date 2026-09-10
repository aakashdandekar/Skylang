
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../include/skylang.h"
#include "../include/sky_python.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Value sky_mod_python;
static bool py_initialized = false;

static void sky_python_ensure_init(void) {
    if (!py_initialized) {
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }
        py_initialized = true;

        PyRun_SimpleString(
            "import sys, os, site\n"
            "try:\n"
            "    user_sp = site.getusersitepackages()\n"
            "    if os.path.isdir(user_sp) and user_sp not in sys.path:\n"
            "        site.addsitedir(user_sp)\n"
            "except Exception:\n"
            "    pass\n"
            "\n"
            "# 1. Active virtualenv or conda environment from environment variables\n"
            "active_env = os.environ.get('VIRTUAL_ENV') or os.environ.get('CONDA_PREFIX')\n"
            "cand_venvs = [active_env] if active_env else []\n"
            "\n"
            "# 2. Scan cwd and parent directories for .venv, venv, env\n"
            "try:\n"
            "    curr = os.path.abspath(os.getcwd())\n"
            "    for _ in range(4):\n"
            "        for vname in ['.venv', 'venv', 'env', '.env']:\n"
            "            vp = os.path.join(curr, vname)\n"
            "            if os.path.isdir(vp) and vp not in cand_venvs:\n"
            "                cand_venvs.append(vp)\n"
            "        parent = os.path.dirname(curr)\n"
            "        if parent == curr: break\n"
            "        curr = parent\n"
            "except Exception:\n"
            "    pass\n"
            "\n"
            "# 3. Add discovered site-packages to sys.path with full .pth support\n"
            "for v in cand_venvs:\n"
            "    if not v: continue\n"
            "    lib_dir = os.path.join(v, 'lib')\n"
            "    if os.path.isdir(lib_dir):\n"
            "        try:\n"
            "            for pyver in os.listdir(lib_dir):\n"
            "                sp = os.path.join(lib_dir, pyver, 'site-packages')\n"
            "                if os.path.isdir(sp):\n"
            "                    site.addsitedir(sp)\n"
            "        except Exception:\n"
            "            pass\n"
            "    sp_win = os.path.join(v, 'Lib', 'site-packages')\n"
            "    if os.path.isdir(sp_win):\n"
            "        site.addsitedir(sp_win)\n"
        );
    }
}

static PyObject* sky_to_py(Value v) {
    sky_python_ensure_init();
    switch (v.type) {
        case VAL_NIL:
            Py_INCREF(Py_None);
            return Py_None;
        case VAL_INT:
            return PyLong_FromLongLong(v.as.i);
        case VAL_DOUBLE:
            return PyFloat_FromDouble(v.as.d);
        case VAL_BOOL:
            return PyBool_FromLong(v.as.b ? 1 : 0);
        case VAL_CHAR: {
            char buf[2] = {v.as.c, '\0'};
            return PyUnicode_FromString(buf);
        }
        case VAL_TYPE:
            return PyUnicode_FromString(sky_type_name(v.as.t));
        case VAL_OBJ: {
            if (!v.as.obj) {
                Py_INCREF(Py_None);
                return Py_None;
            }
            switch (v.as.obj->type) {
                case OBJ_STRING: {
                    ObjString* s = (ObjString*)v.as.obj;
                    return PyUnicode_FromString(s->chars ? s->chars : "");
                }
                case OBJ_LIST: {
                    ObjList* l = (ObjList*)v.as.obj;
                    PyObject* pylist = PyList_New((Py_ssize_t)l->count);
                    for (size_t i = 0; i < l->count; ++i) {
                        PyObject* item = sky_to_py(l->items[i]);
                        PyList_SetItem(pylist, (Py_ssize_t)i, item);
                    }
                    return pylist;
                }
                case OBJ_ARRAY: {
                    ObjArray* a = (ObjArray*)v.as.obj;
                    PyObject* pylist = PyList_New((Py_ssize_t)a->capacity);
                    for (size_t i = 0; i < a->capacity; ++i) {
                        PyObject* item = sky_to_py(a->items[i]);
                        PyList_SetItem(pylist, (Py_ssize_t)i, item);
                    }
                    return pylist;
                }
                case OBJ_TUPLE: {
                    ObjTuple* t = (ObjTuple*)v.as.obj;
                    PyObject* pytuple = PyTuple_New((Py_ssize_t)t->count);
                    for (size_t i = 0; i < t->count; ++i) {
                        PyObject* item = sky_to_py(t->items[i]);
                        PyTuple_SetItem(pytuple, (Py_ssize_t)i, item);
                    }
                    return pytuple;
                }
                case OBJ_DICT: {
                    ObjDict* d = (ObjDict*)v.as.obj;
                    PyObject* pydict = PyDict_New();
                    for (size_t i = 0; i < d->capacity; ++i) {
                        if (d->entries[i].occupied) {
                            PyObject* k = sky_to_py(d->entries[i].key);
                            PyObject* val = sky_to_py(d->entries[i].value);
                            PyDict_SetItem(pydict, k, val);
                            Py_DECREF(k);
                            Py_DECREF(val);
                        }
                    }
                    return pydict;
                }
                case OBJ_FOREIGN: {
                    ObjForeign* f = (ObjForeign*)v.as.obj;
                    if (f->lang == FOREIGN_PYTHON && f->handle) {
                        PyObject* pyobj = (PyObject*)f->handle;
                        Py_INCREF(pyobj);
                        return pyobj;
                    }
                    Py_INCREF(Py_None);
                    return Py_None;
                }
                default:
                    Py_INCREF(Py_None);
                    return Py_None;
            }
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static Value py_to_sky(PyObject* obj) {
    if (!obj || obj == Py_None) {
        return val_nil();
    }
    if (PyBool_Check(obj)) {
        return val_bool(obj == Py_True);
    }
    if (PyLong_Check(obj)) {
        return val_int((int64_t)PyLong_AsLongLong(obj));
    }
    if (PyFloat_Check(obj)) {
        return val_double(PyFloat_AsDouble(obj));
    }
    if (PyUnicode_Check(obj)) {
        const char* s = PyUnicode_AsUTF8(obj);
        return val_string(s ? s : "");
    }
    if (PyList_Check(obj) || PyTuple_Check(obj)) {
        Value res = val_list();
        ObjList* l = as_list(res);
        Py_ssize_t sz = PySequence_Size(obj);
        for (Py_ssize_t i = 0; i < sz; ++i) {
            PyObject* item = PySequence_GetItem(obj, i);
            list_push(l, py_to_sky(item));
            Py_XDECREF(item);
        }
        return res;
    }
    if (PyDict_Check(obj)) {
        Value res = val_dict();
        ObjDict* d = as_dict(res);
        PyObject *key, *val;
        Py_ssize_t pos = 0;
        while (PyDict_Next(obj, &pos, &key, &val)) {
            dict_set(d, py_to_sky(key), py_to_sky(val));
        }
        return res;
    }

    if (PyObject_HasAttrString(obj, "tolist")) {
        PyObject* tolist_fn = PyObject_GetAttrString(obj, "tolist");
        if (tolist_fn) {
            if (PyCallable_Check(tolist_fn)) {
                PyObject* list_obj = PyObject_CallObject(tolist_fn, NULL);
                Py_DECREF(tolist_fn);
                if (list_obj) {
                    Value res = py_to_sky(list_obj);
                    Py_DECREF(list_obj);
                    return res;
                }
            } else {
                Py_DECREF(tolist_fn);
            }
        }
        if (PyErr_Occurred()) PyErr_Clear();
    }

    if (PyObject_HasAttrString(obj, "item")) {
        PyObject* item_fn = PyObject_GetAttrString(obj, "item");
        if (item_fn) {
            if (PyCallable_Check(item_fn)) {
                PyObject* item_obj = PyObject_CallObject(item_fn, NULL);
                Py_DECREF(item_fn);
                if (item_obj) {
                    Value res = py_to_sky(item_obj);
                    Py_DECREF(item_obj);
                    return res;
                }
            } else {
                Py_DECREF(item_fn);
            }
        }
        if (PyErr_Occurred()) PyErr_Clear();
    }

    if (PyNumber_Check(obj)) {
        if (PyLong_Check(obj)) {
            return val_int((int64_t)PyLong_AsLongLong(obj));
        }
        PyObject* f = PyNumber_Float(obj);
        if (f) {
            double d = PyFloat_AsDouble(f);
            Py_DECREF(f);
            return val_double(d);
        }
        if (PyErr_Occurred()) PyErr_Clear();
    }

    const char* type_name = Py_TYPE(obj)->tp_name;
    Py_INCREF(obj);
    return val_foreign(FOREIGN_PYTHON, type_name, (void*)obj, NULL);
}

static bool is_py_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool is_py_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool is_py_reserved_keyword(const char* id) {
    static const char* kws[] = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield", "print", "len", "range", "dict",
        "list", "set", "tuple", "str", "int", "float", "bool", "type"
    };
    for (size_t i = 0; i < sizeof(kws)/sizeof(kws[0]); ++i) {
        if (strcmp(id, kws[i]) == 0) return true;
    }
    return false;
}

static char* sky_dedent_code(const char* code) {
    if (!code) return NULL;
    int min_indent = 999999;
    const char* scan = code;
    bool at_ls = true;
    int ind = 0;
    while (*scan) {
        if (*scan == '\n') {
            scan++;
            at_ls = true;
            ind = 0;
            continue;
        }
        if (at_ls) {
            if (*scan == ' ') { ind++; scan++; continue; }
            if (*scan == '\t') { ind += 4; scan++; continue; }
            if (*scan == '\r') { scan++; continue; }
            if (*scan != '#' && *scan != '\0') {
                if (ind < min_indent) min_indent = ind;
            }
            at_ls = false;
        }
        scan++;
    }
    if (min_indent == 999999 || min_indent == 0) return strdup(code);

    size_t len = strlen(code);
    char* result = (char*)malloc(len + 1);
    char* out = result;
    const char* p = code;
    at_ls = true;
    int skip = 0;

    while (*p) {
        if (*p == '\n') {
            *out++ = *p++;
            at_ls = true;
            skip = 0;
            continue;
        }
        if (at_ls) {
            if ((*p == ' ' || *p == '\t') && skip < min_indent) {
                skip += (*p == '\t') ? 4 : 1;
                p++;
                continue;
            }
            at_ls = false;
        }
        *out++ = *p++;
    }
    *out = '\0';
    return result;
}

void sky_extract_py_declarations(const char* code, const char* file, int base_line, ForeignSymbolTable* table) {
    if (!code) return;
    char* dedented = sky_dedent_code(code);
    const char* p = dedented ? dedented : code;
    int cur_line = base_line;
    bool at_line_start = true;
    int indent_level = 0;

    while (*p) {
        if (*p == '\n') {
            cur_line++;
            p++;
            at_line_start = true;
            indent_level = 0;
            continue;
        }
        if (at_line_start) {
            if (*p == ' ') { indent_level++; p++; continue; }
            if (*p == '\t') { indent_level += 4; p++; continue; }
            at_line_start = false;
        }
        if (isspace((unsigned char)*p)) { p++; continue; }

        if (*p == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }

        if ((p[0] == '"' && p[1] == '"' && p[2] == '"') || (p[0] == '\'' && p[1] == '\'' && p[2] == '\'')) {
            char q = p[0];
            p += 3;
            while (*p && !(p[0] == q && p[1] == q && p[2] == q)) {
                if (*p == '\n') cur_line++;
                if (*p == '\\' && *(p+1)) p += 2;
                else p++;
            }
            if (*p) p += 3;
            continue;
        }
        if (*p == '"' || *p == '\'') {
            char q = *p++;
            while (*p && *p != q) {
                if (*p == '\n') cur_line++;
                if (*p == '\\' && *(p+1)) p += 2;
                else p++;
            }
            if (*p) p += 2;
            continue;
        }

        if (is_py_ident_start(*p)) {
            const char* start = p;
            while (is_py_ident_char(*p)) p++;
            size_t id_len = (size_t)(p - start);
            char ident[128];
            if (id_len >= sizeof(ident)) id_len = sizeof(ident) - 1;
            memcpy(ident, start, id_len);
            ident[id_len] = '\0';

            if (indent_level == 0) {
                if (strcmp(ident, "def") == 0) {
                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (is_py_ident_start(*p)) {
                        const char* fn_start = p;
                        while (is_py_ident_char(*p)) p++;
                        size_t fn_len = (size_t)(p - fn_start);
                        char fn_name[128];
                        if (fn_len >= sizeof(fn_name)) fn_len = sizeof(fn_name) - 1;
                        memcpy(fn_name, fn_start, fn_len);
                        fn_name[fn_len] = '\0';
                        if (!is_py_reserved_keyword(fn_name)) {
                            foreign_symtable_add(table, fn_name, "python", FOREIGN_DECL_FUNCTION, file, cur_line);
                        }
                    }
                    while (*p && *p != '\n') p++;
                    continue;
                } else if (strcmp(ident, "class") == 0) {
                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (is_py_ident_start(*p)) {
                        const char* cl_start = p;
                        while (is_py_ident_char(*p)) p++;
                        size_t cl_len = (size_t)(p - cl_start);
                        char cl_name[128];
                        if (cl_len >= sizeof(cl_name)) cl_len = sizeof(cl_name) - 1;
                        memcpy(cl_name, cl_start, cl_len);
                        cl_name[cl_len] = '\0';
                        if (!is_py_reserved_keyword(cl_name)) {
                            foreign_symtable_add(table, cl_name, "python", FOREIGN_DECL_CLASS, file, cur_line);
                        }
                    }
                    while (*p && *p != '\n') p++;
                    continue;
                } else if (strcmp(ident, "import") == 0) {
                    while (*p && *p != '\n') {
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (!is_py_ident_start(*p)) break;
                        const char* mod_start = p;
                        while (is_py_ident_char(*p) || *p == '.') p++;
                        size_t mod_len = (size_t)(p - mod_start);
                        char mod_name[128];
                        if (mod_len >= sizeof(mod_name)) mod_len = sizeof(mod_name) - 1;
                        memcpy(mod_name, mod_start, mod_len);
                        mod_name[mod_len] = '\0';

                        while (*p && isspace((unsigned char)*p)) p++;
                        if (strncmp(p, "as", 2) == 0 && isspace((unsigned char)p[2])) {
                            p += 2;
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (is_py_ident_start(*p)) {
                                const char* as_start = p;
                                while (is_py_ident_char(*p)) p++;
                                size_t as_len = (size_t)(p - as_start);
                                char as_name[128];
                                if (as_len >= sizeof(as_name)) as_len = sizeof(as_name) - 1;
                                memcpy(as_name, as_start, as_len);
                                as_name[as_len] = '\0';
                                foreign_symtable_add(table, as_name, "python", FOREIGN_DECL_VAR, file, cur_line);
                            }
                        } else {
                            char* dot = strchr(mod_name, '.');
                            if (dot) *dot = '\0';
                            foreign_symtable_add(table, mod_name, "python", FOREIGN_DECL_VAR, file, cur_line);
                        }
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (*p == ',') p++;
                        else break;
                    }
                    while (*p && *p != '\n') p++;
                    continue;
                } else if (strcmp(ident, "from") == 0) {
                    const char* imp_pos = strstr(p, "import");
                    if (imp_pos) {
                        p = imp_pos + 6;
                        while (*p && *p != '\n') {
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (*p == '*') break;
                            if (!is_py_ident_start(*p)) break;
                            const char* sym_start = p;
                            while (is_py_ident_char(*p)) p++;
                            size_t sym_len = (size_t)(p - sym_start);
                            char sym_name[128];
                            if (sym_len >= sizeof(sym_name)) sym_len = sizeof(sym_name) - 1;
                            memcpy(sym_name, sym_start, sym_len);
                            sym_name[sym_len] = '\0';

                            while (*p && isspace((unsigned char)*p)) p++;
                            if (strncmp(p, "as", 2) == 0 && isspace((unsigned char)p[2])) {
                                p += 2;
                                while (*p && isspace((unsigned char)*p)) p++;
                                if (is_py_ident_start(*p)) {
                                    const char* as_start = p;
                                    while (is_py_ident_char(*p)) p++;
                                    size_t as_len = (size_t)(p - as_start);
                                    char as_name[128];
                                    if (as_len >= sizeof(as_name)) as_len = sizeof(as_name) - 1;
                                    memcpy(as_name, as_start, as_len);
                                    as_name[as_len] = '\0';
                                    foreign_symtable_add(table, as_name, "python", FOREIGN_DECL_VAR, file, cur_line);
                                }
                            } else {
                                foreign_symtable_add(table, sym_name, "python", FOREIGN_DECL_VAR, file, cur_line);
                            }
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (*p == ',') p++;
                            else break;
                        }
                    }
                    while (*p && *p != '\n') p++;
                    continue;
                } else if (!is_py_reserved_keyword(ident)) {
                    char targets[16][128];
                    size_t target_count = 0;
                    strncpy(targets[target_count++], ident, 127);
                    targets[0][127] = '\0';

                    const char* scan_p = p;
                    while (*scan_p && isspace((unsigned char)*scan_p) && *scan_p != '\n') scan_p++;
                    
                    while (*scan_p == ',') {
                        scan_p++;
                        while (*scan_p && isspace((unsigned char)*scan_p) && *scan_p != '\n') scan_p++;
                        if (is_py_ident_start(*scan_p)) {
                            const char* t_start = scan_p;
                            while (is_py_ident_char(*scan_p)) scan_p++;
                            size_t t_len = (size_t)(scan_p - t_start);
                            if (t_len >= 128) t_len = 127;
                            if (target_count < 16) {
                                memcpy(targets[target_count], t_start, t_len);
                                targets[target_count][t_len] = '\0';
                                target_count++;
                            }
                            while (*scan_p && isspace((unsigned char)*scan_p) && *scan_p != '\n') scan_p++;
                        } else {
                            break;
                        }
                    }

                    if (*scan_p == '=' && *(scan_p + 1) != '=') {
                        for (size_t ti = 0; ti < target_count; ++ti) {
                            if (!is_py_reserved_keyword(targets[ti])) {
                                foreign_symtable_add(table, targets[ti], "python", FOREIGN_DECL_ASSIGN, file, cur_line);
                            }
                        }
                        p = scan_p;
                    }
                }
            }
        }

        p++;
    }
    if (dedented) free(dedented);
}

Value sky_python_load(const char* module_name) {
    sky_python_ensure_init();
    PyObject* mod = PyImport_ImportModule(module_name);
    if (!mod) {
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        sky_runtime_error("ModuleNotFoundError", "No module named '%s'", module_name);
        return val_nil();
    }
    return val_foreign(FOREIGN_PYTHON, module_name, (void*)mod, NULL);
}

Value sky_python_exec(const char* code) {
    if (!code || !*code) return val_dict();
    sky_python_ensure_init();

    char* dedented = sky_dedent_code(code);
    const char* run_code = dedented ? dedented : code;

    ForeignSymbolTable syms;
    foreign_symtable_init(&syms);
    sky_extract_py_declarations(run_code, "<python>", 1, &syms);

    PyObject* main_mod = PyImport_AddModule("__main__");
    PyObject* global_dict = PyModule_GetDict(main_mod);
    PyObject* res = PyRun_String(run_code, Py_file_input, global_dict, global_dict);
    if (dedented) free(dedented);
    if (!res) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        foreign_symtable_free(&syms);
        return val_dict();
    }
    Py_DECREF(res);

    Value result_dict = val_dict();
    ObjDict* d = as_dict(result_dict);
    for (size_t i = 0; i < syms.count; ++i) {
        const char* sname = syms.items[i].name;
        PyObject* item = PyDict_GetItemString(global_dict, sname);
        if (item) {
            dict_set(d, val_string(sname), py_to_sky(item));
        }
    }

    foreign_symtable_free(&syms);
    return result_dict;
}

void sky_python_set_global(const char* name, Value val) {
    if (!name || !*name) return;
    sky_python_ensure_init();
    PyObject* main_mod = PyImport_AddModule("__main__");
    if (!main_mod) return;
    PyObject* global_dict = PyModule_GetDict(main_mod);
    if (!global_dict) return;
    PyObject* py_val = sky_to_py(val);
    if (py_val) {
        PyDict_SetItemString(global_dict, name, py_val);
        Py_DECREF(py_val);
    }
}

Value sky_python_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f || !f->handle) return val_nil();
    sky_python_ensure_init();

    PyObject* target = (PyObject*)f->handle;
    PyObject* attr = PyObject_GetAttrString(target, name);
    if (!attr) {
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        sky_runtime_error("AttributeError", "module '%s' has no attribute '%s'", f->name ? f->name : "object", name);
        return val_nil();
    }

    if (PyCallable_Check(attr)) {
        PyObject* py_args = PyTuple_New((Py_ssize_t)argc);
        for (int i = 0; i < argc; ++i) {
            PyObject* arg = sky_to_py(argv[i]);
            PyTuple_SetItem(py_args, (Py_ssize_t)i, arg);
        }
        PyObject* result = PyObject_CallObject(attr, py_args);
        Py_DECREF(py_args);

        if (!result && PyErr_Occurred()) {
            if (strcmp(name, "predict") == 0 && argc == 1) {
                PyErr_Clear();
                PyObject* py_2d = NULL;
                if (argv[0].type == VAL_INT || argv[0].type == VAL_DOUBLE) {
                    double num = (argv[0].type == VAL_INT) ? (double)argv[0].as.i : argv[0].as.d;
                    py_2d = PyList_New(1);
                    PyObject* row = PyList_New(1);
                    PyList_SetItem(row, 0, PyFloat_FromDouble(num));
                    PyList_SetItem(py_2d, 0, row);
                } else if (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) {
                    ObjString* s = (ObjString*)argv[0].as.obj;
                    if (s->chars) {
                        char* endptr = NULL;
                        double num = strtod(s->chars, &endptr);
                        while (endptr && isspace((unsigned char)*endptr)) endptr++;
                        if (endptr && *endptr == '\0' && endptr != s->chars) {
                            py_2d = PyList_New(1);
                            PyObject* row = PyList_New(1);
                            PyList_SetItem(row, 0, PyFloat_FromDouble(num));
                            PyList_SetItem(py_2d, 0, row);
                        }
                    }
                } else if (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_LIST) {
                    ObjList* l = (ObjList*)argv[0].as.obj;
                    bool is_1d = true;
                    for (size_t k = 0; k < l->count; ++k) {
                        if (l->items[k].type == VAL_OBJ && l->items[k].as.obj->type == OBJ_LIST) {
                            is_1d = false;
                            break;
                        }
                    }
                    if (is_1d) {
                        py_2d = PyList_New(1);
                        PyObject* row = PyList_New((Py_ssize_t)l->count);
                        for (size_t k = 0; k < l->count; ++k) {
                            PyList_SetItem(row, (Py_ssize_t)k, sky_to_py(l->items[k]));
                        }
                        PyList_SetItem(py_2d, 0, row);
                    }
                }

                if (py_2d) {
                    PyObject* retry_args = PyTuple_New(1);
                    PyTuple_SetItem(retry_args, 0, py_2d);
                    result = PyObject_CallObject(attr, retry_args);
                    Py_DECREF(retry_args);
                }
            }
        }
        Py_DECREF(attr);

        if (!result) {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            return val_nil();
        }
        Value sky_res = py_to_sky(result);
        Py_DECREF(result);
        return sky_res;
    } else {

        Value sky_res = py_to_sky(attr);
        Py_DECREF(attr);
        return sky_res;
    }
}

Value sky_python_get_prop(ObjForeign* f, const char* name) {
    if (!f || !f->handle) return val_nil();
    sky_python_ensure_init();

    PyObject* target = (PyObject*)f->handle;
    PyObject* attr = PyObject_GetAttrString(target, name);
    if (!attr) {
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        return val_nil();
    }
    Value res = py_to_sky(attr);
    Py_DECREF(attr);
    return res;
}

Value sky_python_set_prop(ObjForeign* f, const char* name, Value val) {
    if (!f || !f->handle) return val_nil();
    sky_python_ensure_init();

    PyObject* target = (PyObject*)f->handle;
    PyObject* py_val = sky_to_py(val);
    int ret = PyObject_SetAttrString(target, name, py_val);
    Py_DECREF(py_val);

    if (ret != 0) {
        if (PyErr_Occurred()) PyErr_Print();
        return val_nil();
    }
    return val;
}

static Value py_native_load(int argc, Value* argv) {
    if (argc == 0) return val_nil();
    if (argc == 1) {
        if (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) {
            return sky_python_load(((ObjString*)argv[0].as.obj)->chars);
        }
        return val_nil();
    }

    Value list = val_list();
    ObjList* l = as_list(list);
    for (int i = 0; i < argc; ++i) {
        if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            list_push(l, sky_python_load(((ObjString*)argv[i].as.obj)->chars));
        }
    }
    return list;
}

static Value py_native_exec(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_python_exec(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_python_init(void) {
    sky_python_ensure_init();

    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"), val_function("load", py_native_load, -1));
    dict_set(d, val_string("exec"), val_function("exec", py_native_exec,  1));

    sky_mod_python = mod;
    return mod;
}

