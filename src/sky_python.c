
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../include/sky_python.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    const char* type_name = Py_TYPE(obj)->tp_name;
    Py_INCREF(obj);
    return val_foreign(FOREIGN_PYTHON, type_name, (void*)obj, NULL);
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
    sky_python_ensure_init();
    PyObject* main_mod = PyImport_AddModule("__main__");
    PyObject* global_dict = PyModule_GetDict(main_mod);
    PyObject* res = PyRun_String(code, Py_file_input, global_dict, global_dict);
    if (!res) {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return val_nil();
    }
    Py_DECREF(res);
    return val_nil();
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

