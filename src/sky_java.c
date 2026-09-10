
#include "../include/skylang.h"
#include "../include/sky_java.h"
#include "../include/sky_platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value sky_mod_java;

static int java_file_counter = 0;

static Value parse_bracket_or_brace_list(const char** pp) {
    const char* p = *pp;
    if (*p != '{' && *p != '[') return val_nil();
    char open_ch = *p;
    char close_ch = (open_ch == '{') ? '}' : ']';
    p++;

    Value list_val = val_list();
    ObjList* l = as_list(list_val);

    while (*p && *p != close_ch) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == close_ch || !*p) break;

        if (*p == '{' || *p == '[') {
            list_push(l, parse_bracket_or_brace_list(&p));
        } else if (*p == '"' || *p == '\'') {
            char q = *p++;
            const char* str_start = p;
            while (*p && *p != q) {
                if (*p == '\\' && *(p + 1)) p += 2;
                else p++;
            }
            size_t slen = (size_t)(p - str_start);
            char* sval = (char*)malloc(slen + 1);
            memcpy(sval, str_start, slen);
            sval[slen] = '\0';
            list_push(l, val_string(sval));
            free(sval);
            if (*p == q) p++;
        } else {
            const char* val_start = p;
            while (*p && *p != ',' && *p != close_ch && !isspace((unsigned char)*p)) p++;
            size_t vlen = (size_t)(p - val_start);
            char vstr[64];
            if (vlen >= sizeof(vstr)) vlen = sizeof(vstr) - 1;
            memcpy(vstr, val_start, vlen);
            vstr[vlen] = '\0';
            if (strcmp(vstr, "true") == 0) {
                list_push(l, val_bool(true));
            } else if (strcmp(vstr, "false") == 0) {
                list_push(l, val_bool(false));
            } else if (strchr(vstr, '.') || strchr(vstr, 'e') || strchr(vstr, 'E') ||
                       strchr(vstr, 'f') || strchr(vstr, 'F') || strchr(vstr, 'd') || strchr(vstr, 'D')) {
                list_push(l, val_double(atof(vstr)));
            } else {
                list_push(l, val_int(atoll(vstr)));
            }
        }
    }
    if (*p == close_ch) p++;
    *pp = p;
    return list_val;
}

void sky_extract_java_declarations(const char* code, const char* file, int base_line, ForeignSymbolTable* table) {
    if (!code) return;
    const char* p = code;
    int cur_line = base_line;
    int brace_depth = 0;

    while (*p) {
        if (*p == '\n') { cur_line++; p++; continue; }
        if (isspace((unsigned char)*p)) { p++; continue; }

        if (p[0] == '/' && p[1] == '/') {
            p += 2;
            while (*p && *p != '\n') p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                if (*p == '\n') cur_line++;
                p++;
            }
            if (*p) p += 2;
            continue;
        }
        if (*p == '"' || *p == '\'') {
            char q = *p++;
            while (*p && *p != q) {
                if (*p == '\\' && *(p + 1)) {
                    if (*p == '\n') cur_line++;
                    p += 2;
                } else {
                    if (*p == '\n') cur_line++;
                    p++;
                }
            }
            if (*p == q) p++;
            continue;
        }

        if (*p == '{') { brace_depth++; p++; continue; }
        if (*p == '}') { if (brace_depth > 0) brace_depth--; p++; continue; }

        if (brace_depth <= 1) {
            static const char* java_modifiers[] = {
                "public", "private", "protected", "static", "final", "abstract",
                "synchronized", "transient", "volatile", "native", "strictfp", "default"
            };
            bool skipped_modifier = false;
            for (size_t m = 0; m < sizeof(java_modifiers)/sizeof(java_modifiers[0]); ++m) {
                size_t mlen = strlen(java_modifiers[m]);
                if (strncmp(p, java_modifiers[m], mlen) == 0 && (isspace((unsigned char)p[mlen]) || p[mlen] == '\0')) {
                    p += mlen;
                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    skipped_modifier = true;
                    break;
                }
            }
            if (skipped_modifier) continue;

            static const char* java_ctrl_keywords[] = {
                "if", "else", "while", "for", "do", "switch", "case", "break", "continue",
                "return", "try", "catch", "finally", "throw", "throws", "new", "instanceof",
                "assert", "import", "package", "class", "interface", "enum", "record"
            };
            bool is_ctrl = false;
            for (size_t c = 0; c < sizeof(java_ctrl_keywords)/sizeof(java_ctrl_keywords[0]); ++c) {
                size_t clen = strlen(java_ctrl_keywords[c]);
                if (strncmp(p, java_ctrl_keywords[c], clen) == 0 && (!isalnum((unsigned char)p[clen]) && p[clen] != '_')) {
                    is_ctrl = true;
                    p += clen;
                    break;
                }
            }
            if (is_ctrl) continue;

            if (isalpha((unsigned char)*p) || *p == '_') {
                while (isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '[' || *p == ']') {
                    p++;
                }
                if (*p == '<') {
                    int tdepth = 1;
                    p++;
                    while (*p && tdepth > 0) {
                        if (*p == '<') tdepth++;
                        else if (*p == '>') tdepth--;
                        p++;
                    }
                }
                while (*p == '[' || *p == ']' || isspace((unsigned char)*p)) {
                    if (*p == '\n') cur_line++;
                    p++;
                }

                while (isalpha((unsigned char)*p) || *p == '_') {
                    const char* id_start = p;
                    while (isalnum((unsigned char)*p) || *p == '_') p++;
                    size_t id_len = (size_t)(p - id_start);
                    char name[128];
                    if (id_len >= sizeof(name)) id_len = sizeof(name) - 1;
                    memcpy(name, id_start, id_len);
                    name[id_len] = '\0';

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }

                    if (*p == '[') {
                        while (*p && *p != ']') p++;
                        if (*p == ']') p++;
                        foreign_symtable_add(table, name, "java", FOREIGN_DECL_VAR, file, cur_line);
                    } else if (*p == '(') {
                        foreign_symtable_add(table, name, "java", FOREIGN_DECL_FUNCTION, file, cur_line);
                        int pdepth = 1;
                        p++;
                        while (*p && pdepth > 0) {
                            if (*p == '(') pdepth++;
                            else if (*p == ')') pdepth--;
                            p++;
                        }
                        while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                        if (*p == '{') {
                            int bdepth = 1;
                            p++;
                            while (*p && bdepth > 0) {
                                if (*p == '{') bdepth++;
                                else if (*p == '}') bdepth--;
                                p++;
                            }
                        }
                        break;
                    } else {
                        foreign_symtable_add(table, name, "java", FOREIGN_DECL_VAR, file, cur_line);
                    }

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (*p == '=') {
                        p++;
                        int paren = 0, bracket = 0, brace = 0;
                        while (*p) {
                            if (*p == '(') paren++;
                            else if (*p == ')') { if (paren > 0) paren--; }
                            else if (*p == '[') bracket++;
                            else if (*p == ']') { if (bracket > 0) bracket--; }
                            else if (*p == '{') brace++;
                            else if (*p == '}') { if (brace > 0) brace--; }
                            else if (*p == '"' || *p == '\'') {
                                char q = *p++;
                                while (*p && *p != q) {
                                    if (*p == '\\' && *(p + 1)) p += 2;
                                    else p++;
                                }
                                if (*p == q) p++;
                                continue;
                            } else if (paren == 0 && bracket == 0 && brace == 0) {
                                if (*p == ',' || *p == ';' || *p == '\n') break;
                            }
                            p++;
                        }
                    }

                    while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                    if (*p == ',') {
                        p++;
                        while (*p && isspace((unsigned char)*p)) { if (*p == '\n') cur_line++; p++; }
                        continue;
                    }
                    if (*p == ';') { p++; break; }
                    break;
                }
                continue;
            }
        }

        p++;
    }
}

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
    char tmp_dir[512];
    sky_get_temp_dir(tmp_dir, sizeof(tmp_dir));
    char java_file[2048];
    snprintf(java_file, sizeof(java_file), "%s/%s.java", tmp_dir, class_name);
    FILE* f = fopen(java_file, "w");
    if (!f) return strdup("null");
    fputs(java_code, f);
    fclose(f);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "java \"%s\" 2>%s", java_file, SKY_DEV_NULL);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        sky_unlink(java_file);
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
    sky_unlink(java_file);

    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

Value sky_java_load(const char* class_name) {
    return val_foreign(FOREIGN_JAVA, class_name, strdup(class_name), NULL);
}

Value sky_java_exec(const char* code) {
    if (!code || !*code) return val_dict();

    ForeignSymbolTable syms;
    foreign_symtable_init(&syms);
    sky_extract_java_declarations(code, "<java>", 1, &syms);

    Value result_dict = val_dict();
    ObjDict* d = as_dict(result_dict);

    for (size_t i = 0; i < syms.count; ++i) {
        const char* sname = syms.items[i].name;
        if (syms.items[i].kind == FOREIGN_DECL_FUNCTION) continue;

        const char* p = strstr(code, sname);
        if (p) {
            p += strlen(sname);
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '[') {
                while (*p && *p != ']') p++;
                if (*p == ']') p++;
                while (*p && isspace((unsigned char)*p)) p++;
            }
            if (*p == '=') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;

                if (strncmp(p, "new ", 4) == 0) {
                    while (*p && *p != '{' && *p != ';' && *p != '\n') p++;
                }

                if (*p == '{' || *p == '[') {
                    Value list_val = parse_bracket_or_brace_list(&p);
                    dict_set(d, val_string(sname), list_val);
                } else if (*p == '"') {
                    p++;
                    const char* str_start = p;
                    while (*p && *p != '"') {
                        if (*p == '\\' && *(p+1)) p += 2;
                        else p++;
                    }
                    size_t slen = (size_t)(p - str_start);
                    char* sval = (char*)malloc(slen + 1);
                    memcpy(sval, str_start, slen);
                    sval[slen] = '\0';
                    dict_set(d, val_string(sname), val_string(sval));
                    free(sval);
                } else {
                    const char* val_start = p;
                    while (*p && *p != ';' && *p != ',' && !isspace((unsigned char)*p)) p++;
                    size_t vlen = (size_t)(p - val_start);
                    char vstr[64];
                    if (vlen >= sizeof(vstr)) vlen = sizeof(vstr) - 1;
                    memcpy(vstr, val_start, vlen);
                    vstr[vlen] = '\0';
                    if (strcmp(vstr, "true") == 0) {
                        dict_set(d, val_string(sname), val_bool(true));
                    } else if (strcmp(vstr, "false") == 0) {
                        dict_set(d, val_string(sname), val_bool(false));
                    } else if (strchr(vstr, '.') || strchr(vstr, 'e') || strchr(vstr, 'E') ||
                               strchr(vstr, 'f') || strchr(vstr, 'F') || strchr(vstr, 'd') || strchr(vstr, 'D')) {
                        dict_set(d, val_string(sname), val_double(atof(vstr)));
                    } else {
                        dict_set(d, val_string(sname), val_int(atoll(vstr)));
                    }
                }
            }
        }
    }

    foreign_symtable_free(&syms);

    char class_name[256];
    snprintf(class_name, sizeof(class_name), "SkyJavaRunner_%d_%d", (int)sky_getpid(), ++java_file_counter);

    char full[8192];
    snprintf(full, sizeof(full),
             "public class %s {\n"
             "    public static void main(String[] args) throws Exception {\n"
             "        %s\n"
             "    }\n"
             "}\n",
             class_name, code);
    char* out = run_java_runner(class_name, full);
    if (out) {
        if (*out && strcmp(out, "null") != 0) {
            printf("%s\n", out);
            fflush(stdout);
        }
        free(out);
    }
    return result_dict;
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

