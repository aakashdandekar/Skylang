#define _GNU_SOURCE
#include "../include/skylang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Traceback (most recent call last):\n");
        fprintf(stderr, "  File \"%s\", line 1, in <main>\n", path);
        fprintf(stderr, "FileNotFoundError: [Errno 2] No such file or directory: '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        fprintf(stderr, "MemoryError: Failed to allocate file read buffer\n");
        return NULL;
    }
    size_t read_bytes = fread(buffer, 1, size, f);
    buffer[read_bytes] = '\0';
    fclose(f);
    return buffer;
}

static void get_project_root(char* out_root, size_t max_len) {
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char* dir = dirname(exe_path);

        if (strstr(dir, "/bin")) {
            char* up = dirname(dir);
            snprintf(out_root, max_len, "%s", up);
            return;
        }
        snprintf(out_root, max_len, "%s", dir);
        return;
    }

    snprintf(out_root, max_len, ".");
}

static void translate_and_print_gcc_errors(const char* gcc_output, const char* default_sky_file) {
    if (!gcc_output || gcc_output[0] == '\0') {
        fprintf(stderr, "Traceback (most recent call last):\n");
        fprintf(stderr, "  File \"%s\", line 1, in <main>\n", default_sky_file ? default_sky_file : "<main>");
        fprintf(stderr, "CompileError: compilation failed\n");
        return;
    }

    char* copy = strdup(gcc_output);
    if (!copy) return;

    char* line = strtok(copy, "\n");
    int error_count = 0;

    while (line && error_count < 2) {

        if (strstr(line, "note:") || strstr(line, "In function") || strstr(line, "at top level")) {
            line = strtok(NULL, "\n");
            continue;
        }

        char* err_pos = strstr(line, ": error:");
        bool is_fatal = false;
        if (!err_pos) {
            err_pos = strstr(line, ": fatal error:");
            if (err_pos) is_fatal = true;
        }

        if (err_pos) {
            char file_part[1024] = "";
            int line_num = 1;
            int col_num = 1;

            size_t prefix_len = err_pos - line;
            char prefix[1024];
            if (prefix_len < sizeof(prefix)) {
                memcpy(prefix, line, prefix_len);
                prefix[prefix_len] = '\0';
                char* p1 = strrchr(prefix, ':');
                if (p1) {
                    *p1 = '\0';
                    col_num = atoi(p1 + 1);
                    char* p2 = strrchr(prefix, ':');
                    if (p2) {
                        *p2 = '\0';
                        line_num = atoi(p2 + 1);
                        snprintf(file_part, sizeof(file_part), "%s", prefix);
                    } else {
                        line_num = col_num;
                        col_num = 1;
                        snprintf(file_part, sizeof(file_part), "%s", prefix);
                    }
                } else {
                    snprintf(file_part, sizeof(file_part), "%s", prefix);
                }
            }

            const char* target_sky_file = (file_part[0] != '\0' && !strstr(file_part, "/tmp/skylang_"))
                                          ? file_part : default_sky_file;

            char* msg = err_pos + (is_fatal ? strlen(": fatal error:") : strlen(": error:"));
            while (*msg == ' ') msg++;

            char exc_type[64] = "CompileError";
            char exc_msg[512] = "";

            char* var_start = strstr(msg, "sky_var_");
            char* fn_start = strstr(msg, "sky_fn_");
            char* mod_start = strstr(msg, "sky_mod_");

            if (strstr(msg, "undeclared") || strstr(msg, "has not been declared")) {
                if (var_start) {
                    snprintf(exc_type, sizeof(exc_type), "NameError");
                    char ident[128];
                    size_t k = 0;
                    char* p = var_start + strlen("sky_var_");
                    while (*p && (*p == '_' || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) {
                        if (k + 1 < sizeof(ident)) ident[k++] = *p;
                        p++;
                    }
                    ident[k] = '\0';
                    snprintf(exc_msg, sizeof(exc_msg), "name '%s' is not defined", ident);
                } else if (fn_start) {
                    snprintf(exc_type, sizeof(exc_type), "NameError");
                    char ident[128];
                    size_t k = 0;
                    char* p = fn_start + strlen("sky_fn_");
                    while (*p && (*p == '_' || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) {
                        if (k + 1 < sizeof(ident)) ident[k++] = *p;
                        p++;
                    }
                    ident[k] = '\0';
                    snprintf(exc_msg, sizeof(exc_msg), "name '%s' is not defined", ident);
                } else if (mod_start) {
                    snprintf(exc_type, sizeof(exc_type), "ModuleNotFoundError");
                    char ident[128];
                    size_t k = 0;
                    char* p = mod_start + strlen("sky_mod_");
                    while (*p && (*p == '_' || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) {
                        if (k + 1 < sizeof(ident)) ident[k++] = *p;
                        p++;
                    }
                    ident[k] = '\0';
                    snprintf(exc_msg, sizeof(exc_msg), "No module named '%s'", ident);
                } else {
                    snprintf(exc_type, sizeof(exc_type), "NameError");
                    snprintf(exc_msg, sizeof(exc_msg), "undefined identifier");
                }
            } else if (strstr(msg, "No such file or directory")) {
                snprintf(exc_type, sizeof(exc_type), "ModuleNotFoundError");
                char* colon = strchr(msg, ':');
                if (colon) *colon = '\0';
                snprintf(exc_msg, sizeof(exc_msg), "No such header or module '%s'", msg);
            } else if (strstr(msg, "too few arguments") || strstr(msg, "too many arguments")) {
                snprintf(exc_type, sizeof(exc_type), "TypeError");
                snprintf(exc_msg, sizeof(exc_msg), "function call argument count mismatch");
            } else if (strstr(msg, "expected")) {
                snprintf(exc_type, sizeof(exc_type), "SyntaxError");
                snprintf(exc_msg, sizeof(exc_msg), "syntax error in statement");
            } else {
                snprintf(exc_type, sizeof(exc_type), "CompileError");
                snprintf(exc_msg, sizeof(exc_msg), "invalid expression or statement");
            }

            fprintf(stderr, "Traceback (most recent call last):\n");
            fprintf(stderr, "  File \"%s\", line %d, in <main>\n", target_sky_file ? target_sky_file : "<main>", line_num);

            if (target_sky_file && strcmp(target_sky_file, "<main>") != 0 && strcmp(target_sky_file, "<input>") != 0 && strcmp(target_sky_file, "<repl>") != 0) {
                FILE* sf = fopen(target_sky_file, "r");
                if (sf) {
                    char sline[1024];
                    int cur_l = 0;
                    while (fgets(sline, sizeof(sline), sf)) {
                        cur_l++;
                        if (cur_l == line_num) {
                            size_t slen = strlen(sline);
                            while (slen > 0 && (sline[slen - 1] == '\n' || sline[slen - 1] == '\r')) {
                                sline[--slen] = '\0';
                            }
                            char* p = sline;
                            while (*p == ' ' || *p == '\t') p++;
                            if (*p != '\0') {
                                fprintf(stderr, "    %s\n", p);
                            }
                            break;
                        }
                    }
                    fclose(sf);
                }
            }

            fprintf(stderr, "%s: %s\n", exc_type, exc_msg);
            error_count++;
        }

        line = strtok(NULL, "\n");
    }

    if (error_count == 0) {
        fprintf(stderr, "Traceback (most recent call last):\n");
        fprintf(stderr, "  File \"%s\", line 1, in <main>\n", default_sky_file ? default_sky_file : "<main>");
        fprintf(stderr, "CompileError: compilation failed\n");
    }

    free(copy);
}

static int compile_c_to_binary(const char* c_source, const char* out_bin, const char* sky_file) {
    char root[1024];
    get_project_root(root, sizeof(root));

    char inc_path[2048];
    char rt_path[2048];
    char stdlib_path[2048];
    char py_path[2048];
    char js_path[2048];
    char cpp_path[2048];
    char java_path[2048];
    char go_path[2048];
    char rust_path[2048];

    char lexer_path[2048];
    char parser_path[2048];
    char compiler_path[2048];
    char vm_path[2048];

    snprintf(inc_path, sizeof(inc_path), "%s/include", root);
    snprintf(rt_path, sizeof(rt_path), "%s/src/runtime.c", root);
    snprintf(stdlib_path, sizeof(stdlib_path), "%s/src/sky_stdlib.c", root);
    snprintf(lexer_path, sizeof(lexer_path), "%s/src/lexer.c", root);
    snprintf(parser_path, sizeof(parser_path), "%s/src/parser.c", root);
    snprintf(compiler_path, sizeof(compiler_path), "%s/src/compiler.c", root);
    snprintf(vm_path, sizeof(vm_path), "%s/src/vm.c", root);
    snprintf(py_path, sizeof(py_path), "%s/src/sky_python.c", root);
    snprintf(js_path, sizeof(js_path), "%s/src/sky_js.c", root);
    snprintf(cpp_path, sizeof(cpp_path), "%s/src/sky_cpp.c", root);
    snprintf(java_path, sizeof(java_path), "%s/src/sky_java.c", root);
    snprintf(go_path, sizeof(go_path), "%s/src/sky_go.c", root);
    snprintf(rust_path, sizeof(rust_path), "%s/src/sky_rust.c", root);

    char c_file_path[1024];
    snprintf(c_file_path, sizeof(c_file_path), "/tmp/skylang_%d.c", getpid());

    FILE* f = fopen(c_file_path, "w");
    if (!f) {
        fprintf(stderr, "Traceback (most recent call last):\n");
        fprintf(stderr, "  File \"%s\", line 1, in <main>\n", sky_file ? sky_file : "<main>");
        fprintf(stderr, "IOError: Failed to create temporary compilation unit\n");
        return 1;
    }
    fputs(c_source, f);
    fclose(f);

    char cmd[32768];
    snprintf(cmd, sizeof(cmd),
             "gcc -O2 -I%s $(pkg-config --cflags python3-embed 2>/dev/null || pkg-config --cflags python3 2>/dev/null || echo \"-I/usr/include/python3.14\") "
             "%s %s %s %s %s %s %s %s %s %s %s %s %s -lgc -lm -ldl $(pkg-config --libs python3-embed 2>/dev/null || pkg-config --libs python3 2>/dev/null || echo \"-lpython3.14\") -o %s 2>&1",
             inc_path, c_file_path, rt_path, stdlib_path, lexer_path, parser_path, compiler_path, vm_path, py_path, js_path, cpp_path, java_path, go_path, rust_path, out_bin);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        unlink(c_file_path);
        return 1;
    }

    char gcc_buf[65536];
    size_t total_read = 0;
    char chunk[1024];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        size_t clen = strlen(chunk);
        if (total_read + clen + 1 < sizeof(gcc_buf)) {
            memcpy(gcc_buf + total_read, chunk, clen);
            total_read += clen;
            gcc_buf[total_read] = '\0';
        }
    }
    int status = pclose(pipe);
    unlink(c_file_path);

    if (status != 0) {
        translate_and_print_gcc_errors(gcc_buf, sky_file);
        return 1;
    }

    return 0;
}

static int cmd_run(const char* sky_file) {
    char* source = read_file(sky_file);
    if (!source) return 1;

    Parser parser;
    parser_init(&parser, source, sky_file);
    AstNode* ast = parse_program(&parser);
    if (parser.had_error) {
        free(source);
        return 1;
    }

    char* c_code = codegen_emit_c(ast, sky_file);
    free(source);

    char bin_path[1024];
    snprintf(bin_path, sizeof(bin_path), "/tmp/skylang_%d.bin", getpid());

    int comp_res = compile_c_to_binary(c_code, bin_path, sky_file);
    free(c_code);

    if (comp_res != 0) {
        return 1;
    }

    int run_res = system(bin_path);
    unlink(bin_path);
    return WEXITSTATUS(run_res);
}

static int cmd_build(const char* sky_file, const char* out_bin) {
    char* source = read_file(sky_file);
    if (!source) return 1;

    Parser parser;
    parser_init(&parser, source, sky_file);
    AstNode* ast = parse_program(&parser);
    if (parser.had_error) {
        free(source);
        return 1;
    }

    char* c_code = codegen_emit_c(ast, sky_file);
    free(source);

    int res = compile_c_to_binary(c_code, out_bin, sky_file);
    free(c_code);

    return res;
}

static void print_help(const char* prog) {
    printf("Skylang Compiler and Toolchain\n");
    printf("Usage:\n");
    printf("  %s run <file.sky>               Compile and run a Skylang program\n", prog);
    printf("  %s build <file.sky> [-o <bin>]  Compile Skylang program to native binary (default: <file>)\n", prog);
    printf("  %s <file.sky>                   Short for 'run <file.sky>'\n", prog);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help(argv[0]);
        return 0;
    }

    const char* command = argv[1];

    if (strcmp(command, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing filename for 'run'\n");
            return 1;
        }
        return cmd_run(argv[2]);
    }

    if (strcmp(command, "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing filename for 'build'\n");
            return 1;
        }
        const char* sky_file = argv[2];
        char default_bin[1024];
        const char* last_slash = strrchr(sky_file, '/');
        const char* bname = last_slash ? last_slash + 1 : sky_file;
        snprintf(default_bin, sizeof(default_bin), "%s", bname);
        char* dot = strrchr(default_bin, '.');
        if (dot && (strcmp(dot, ".sky") == 0 || strcmp(dot, ".skylang") == 0)) {
            *dot = '\0';
        }

        const char* out_bin = default_bin;
        for (int i = 3; i < argc; ++i) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                out_bin = argv[++i];
            }
        }
        return cmd_build(sky_file, out_bin);
    }

    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    return cmd_run(argv[1]);
}

