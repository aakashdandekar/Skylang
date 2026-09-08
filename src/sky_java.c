
#include "../include/sky_java.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gc.h>

Value sky_mod_java;

static int java_file_counter = 0;

static Value parse_java_output(char* out) {
    if (!out || !*out || strcmp(out, "null") == 0) {
        if (out) free(out);
        return val_nil();
    }
    if (strcmp(out, "true") == 0) { free(out); return val_bool(true); }
    if (strcmp(out, "false") == 0) { free(out); return val_bool(false); }

    char* endptr = NULL;
    double d = strtod(out, &endptr);
    if (endptr && *endptr == '\0') {
        if (strchr(out, '.') == NULL && strchr(out, 'e') == NULL && strchr(out, 'E') == NULL) {
            long long ll = strtoll(out, NULL, 10);
            free(out);
            return val_int((int64_t)ll);
        }
        free(out);
        return val_double(d);
    }

    Value res = val_string(out);
    free(out);
    return res;
}

static char* run_java_runner(const char* class_name, const char* java_code) {
    char java_file[1024];
    snprintf(java_file, sizeof(java_file), "/tmp/%s.java", class_name);
    FILE* f = fopen(java_file, "w");
    if (!f) return strdup("null");
    fputs(java_code, f);
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "java %s 2>/dev/null", java_file);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        unlink(java_file);
        return strdup("null");
    }

    size_t cap = 512, len = 0;
    char* buf = (char*)malloc(cap);
    char chunk[256];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        size_t clen = strlen(chunk);
        while (len + clen + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf + len, chunk, clen);
        len += clen;
    }
    buf[len] = '\0';
    pclose(pipe);
    unlink(java_file);

    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

Value sky_java_load(const char* class_name) {
    return val_foreign(FOREIGN_JAVA, class_name, strdup(class_name), NULL);
}

Value sky_java_exec(const char* code) {
    char class_name[256];
    snprintf(class_name, sizeof(class_name), "SkyJavaRunner_%d_%d", getpid(), ++java_file_counter);

    char full[8192];
    snprintf(full, sizeof(full),
             "public class %s {\n"
             "    public static void main(String[] args) throws Exception {\n"
             "        %s\n"
             "    }\n"
             "}\n",
             class_name, code);
    char* out = run_java_runner(class_name, full);
    if (out) free(out);
    return val_nil();
}

Value sky_java_call_method(ObjForeign* f, const char* name, int argc, Value* argv) {
    if (!f || !f->name) return val_nil();

    char args_str[4096] = "";
    for (int i = 0; i < argc; ++i) {
        if (i > 0) strcat(args_str, ", ");
        if (argv[i].type == VAL_INT) {
            char num[64]; snprintf(num, sizeof(num), "%lldL", (long long)argv[i].as.i);
            strcat(args_str, num);
        } else if (argv[i].type == VAL_DOUBLE) {
            char num[64]; snprintf(num, sizeof(num), "%g", argv[i].as.d);
            strcat(args_str, num);
        } else if (argv[i].type == VAL_BOOL) {
            strcat(args_str, argv[i].as.b ? "true" : "false");
        } else if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            strcat(args_str, "\"");
            strcat(args_str, ((ObjString*)argv[i].as.obj)->chars);
            strcat(args_str, "\"");
        } else {
            strcat(args_str, "null");
        }
    }

    char class_name[256];
    snprintf(class_name, sizeof(class_name), "SkyJavaRunner_%d_%d", getpid(), ++java_file_counter);

    char code[8192];
    snprintf(code, sizeof(code),
             "public class %s {\n"
             "    public static void main(String[] args) {\n"
             "        try {\n"
             "            var res = %s.%s(%s);\n"
             "            System.out.println(res);\n"
             "        } catch (Exception e) {\n"
             "            System.out.println(\"null\");\n"
             "        }\n"
             "    }\n"
             "}\n",
             class_name, f->name, name, args_str);

    char* out = run_java_runner(class_name, code);
    return parse_java_output(out);
}

Value sky_java_get_prop(ObjForeign* f, const char* name) {
    if (!f || !f->name) return val_nil();

    char class_name[256];
    snprintf(class_name, sizeof(class_name), "SkyJavaRunner_%d_%d", getpid(), ++java_file_counter);

    char code[4096];
    snprintf(code, sizeof(code),
             "public class %s {\n"
             "    public static void main(String[] args) {\n"
             "        try { System.out.println(%s.%s); } catch(Exception e) { System.out.println(\"null\"); }\n"
             "    }\n"
             "}\n",
             class_name, f->name, name);
    char* out = run_java_runner(class_name, code);
    return parse_java_output(out);
}

Value sky_java_set_prop(ObjForeign* f, const char* name, Value val) {
    (void)f; (void)name; (void)val;
    return val_nil();
}

static Value java_native_load(int argc, Value* argv) {
    if (argc == 0) return val_nil();
    if (argc == 1) {
        if (argv[0].type == VAL_OBJ && argv[0].as.obj->type == OBJ_STRING) {
            return sky_java_load(((ObjString*)argv[0].as.obj)->chars);
        }
        return val_nil();
    }
    Value list = val_list();
    ObjList* l = as_list(list);
    for (int i = 0; i < argc; ++i) {
        if (argv[i].type == VAL_OBJ && argv[i].as.obj->type == OBJ_STRING) {
            list_push(l, sky_java_load(((ObjString*)argv[i].as.obj)->chars));
        }
    }
    return list;
}

static Value java_native_exec(int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_OBJ || argv[0].as.obj->type != OBJ_STRING) return val_nil();
    return sky_java_exec(((ObjString*)argv[0].as.obj)->chars);
}

Value sky_java_init(void) {
    Value mod = val_dict();
    ObjDict* d = as_dict(mod);
    dict_set(d, val_string("load"), val_function("load", java_native_load, -1));
    dict_set(d, val_string("exec"), val_function("exec", java_native_exec,  1));

    sky_mod_java = mod;
    return mod;
}

