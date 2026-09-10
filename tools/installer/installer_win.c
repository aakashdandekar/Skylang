/**
 * Skylang Windows Native Installer & Uninstaller
 *
 * Standalone Win32 C installer for Skylang.
 * - Installs Skylang toolchain, runtime, headers, and standard library.
 * - Registers user PATH in Windows Registry (HKCU\Environment).
 * - Broadcasts environment change across the Windows shell.
 * - Registers .sky and .skylang file associations.
 * - Adds Skylang to Windows Add/Remove Programs.
 * - Creates standalone uninstaller.
 * - Supports silent/headless installation via --silent or -s.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SKYLANG_VERSION "1.0.0"
#define SKYLANG_APP_NAME "Skylang Programming Language"
#define SKYLANG_PUBLISHER "Aakash Dandekar"
#define SKYLANG_URL "https://github.com/skylang-lang/skylang"

#ifdef _WIN32
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

static HANDLE hConsole;

static void set_color(WORD color) {
    if (!hConsole) hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

static void reset_color(void) {
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static void print_banner(void) {
    set_color(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("======================================================================\n");
    printf("        ____  _           _                       \n");
    printf("       / ___|| | _____   | | __ _ _ __   __ _     \n");
    printf("       \\___ \\| |/ / | | | | |/ _` | '_ \\ / _` |    \n");
    printf("        ___) |   <| |_| | | | (_| | | | | (_| |    \n");
    printf("       |____/|_|\\_\\\\__, | |_|\\__,_|_| |_|\\__, |    \n");
    printf("                   |___/                 |___/     \n");
    printf("          Skylang Toolchain Windows Installer (v%s)  \n", SKYLANG_VERSION);
    printf("======================================================================\n\n");
    reset_color();
}

static bool create_directory_recursive(const char* path) {
    char temp[MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (len == 0) return false;
    if (temp[len - 1] == '\\' || temp[len - 1] == '/') temp[len - 1] = '\0';

    for (p = temp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            char ch = *p;
            *p = '\0';
            CreateDirectoryA(temp, NULL);
            *p = ch;
        }
    }
    return CreateDirectoryA(temp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool copy_file_force(const char* src, const char* dst) {
    SetFileAttributesA(dst, FILE_ATTRIBUTE_NORMAL);
    return CopyFileA(src, dst, FALSE) != 0;
}

static bool copy_dir_recursive(const char* src, const char* dst) {
    create_directory_recursive(dst);
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*.*", src);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char src_sub[MAX_PATH];
        char dst_sub[MAX_PATH];
        snprintf(src_sub, sizeof(src_sub), "%s\\%s", src, fd.cFileName);
        snprintf(dst_sub, sizeof(dst_sub), "%s\\%s", dst, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            copy_dir_recursive(src_sub, dst_sub);
        } else {
            copy_file_force(src_sub, dst_sub);
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return true;
}

static bool add_to_user_path(const char* bin_dir) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    char current_path[32768] = "";
    DWORD path_len = sizeof(current_path);
    DWORD type = REG_EXPAND_SZ;

    if (RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)current_path, &path_len) != ERROR_SUCCESS) {
        current_path[0] = '\0';
    }

    // Check if already in PATH
    if (strstr(current_path, bin_dir) != NULL) {
        RegCloseKey(hKey);
        return true;
    }

    char new_path[32768];
    if (strlen(current_path) > 0) {
        snprintf(new_path, sizeof(new_path), "%s;%s", bin_dir, current_path);
    } else {
        snprintf(new_path, sizeof(new_path), "%s", bin_dir);
    }

    LONG set_res = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)new_path, (DWORD)strlen(new_path) + 1);
    RegCloseKey(hKey);

    if (set_res == ERROR_SUCCESS) {
        // Broadcast environment update to all windows
        DWORD_PTR result;
        SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 2000, &result);
        return true;
    }
    return false;
}

static bool remove_from_user_path(const char* bin_dir) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    char current_path[32768] = "";
    DWORD path_len = sizeof(current_path);
    DWORD type = REG_EXPAND_SZ;

    if (RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)current_path, &path_len) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }

    if (strstr(current_path, bin_dir) == NULL) {
        RegCloseKey(hKey);
        return true;
    }

    // Reconstruct PATH without bin_dir
    char new_path[32768] = "";
    char* token = strtok(current_path, ";");
    bool first = true;

    while (token) {
        if (_stricmp(token, bin_dir) != 0 && strlen(token) > 0) {
            if (!first) strcat(new_path, ";");
            strcat(new_path, token);
            first = false;
        }
        token = strtok(NULL, ";");
    }

    LONG set_res = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)new_path, (DWORD)strlen(new_path) + 1);
    RegCloseKey(hKey);

    if (set_res == ERROR_SUCCESS) {
        DWORD_PTR result;
        SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 2000, &result);
        return true;
    }
    return false;
}

static void register_file_associations(const char* sky_exe) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.sky", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)"SkylangSourceFile", (DWORD)strlen("SkylangSourceFile") + 1);
        RegCloseKey(hKey);
    }
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.skylang", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)"SkylangSourceFile", (DWORD)strlen("SkylangSourceFile") + 1);
        RegCloseKey(hKey);
    }
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\SkylangSourceFile", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)"Skylang Source File", (DWORD)strlen("Skylang Source File") + 1);
        RegCloseKey(hKey);
    }
    char open_cmd[MAX_PATH + 32];
    snprintf(open_cmd, sizeof(open_cmd), "\"%s\" \"%%1\"", sky_exe);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\SkylangSourceFile\\shell\\open\\command", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)open_cmd, (DWORD)strlen(open_cmd) + 1);
        RegCloseKey(hKey);
    }
    char run_cmd[MAX_PATH + 32];
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\" run \"%%1\"", sky_exe);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\SkylangSourceFile\\shell\\run\\command", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)run_cmd, (DWORD)strlen(run_cmd) + 1);
        RegCloseKey(hKey);
    }
}

static void unregister_file_associations(void) {
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\Classes\\.sky");
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\Classes\\.skylang");
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\Classes\\SkylangSourceFile");
}

static void register_uninstall_entry(const char* install_dir, const char* uninstaller_exe) {
    HKEY hKey;
    const char* uninst_key = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Skylang";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, uninst_key, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "DisplayName", 0, REG_SZ, (const BYTE*)SKYLANG_APP_NAME, (DWORD)strlen(SKYLANG_APP_NAME) + 1);
        RegSetValueExA(hKey, "DisplayVersion", 0, REG_SZ, (const BYTE*)SKYLANG_VERSION, (DWORD)strlen(SKYLANG_VERSION) + 1);
        RegSetValueExA(hKey, "Publisher", 0, REG_SZ, (const BYTE*)SKYLANG_PUBLISHER, (DWORD)strlen(SKYLANG_PUBLISHER) + 1);
        RegSetValueExA(hKey, "HelpLink", 0, REG_SZ, (const BYTE*)SKYLANG_URL, (DWORD)strlen(SKYLANG_URL) + 1);
        RegSetValueExA(hKey, "InstallLocation", 0, REG_SZ, (const BYTE*)install_dir, (DWORD)strlen(install_dir) + 1);
        RegSetValueExA(hKey, "UninstallString", 0, REG_SZ, (const BYTE*)uninstaller_exe, (DWORD)strlen(uninstaller_exe) + 1);
        RegCloseKey(hKey);
    }
}

static void unregister_uninstall_entry(void) {
    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Skylang");
}

static bool check_gcc_available(void) {
    return system("gcc --version >nul 2>&1") == 0;
}

static int run_installer(const char* custom_dest, bool silent) {
    if (!silent) print_banner();

    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char src_dir[MAX_PATH];
    snprintf(src_dir, sizeof(src_dir), "%s", exe_path);
    char* last_slash = strrchr(src_dir, '\\');
    if (last_slash) *last_slash = '\0';

    char repo_root[MAX_PATH];
    snprintf(repo_root, sizeof(repo_root), "%s", src_dir);
    if (strstr(repo_root, "\\tools\\installer") || strstr(repo_root, "\\bin")) {
        char* p = strstr(repo_root, "\\tools\\installer");
        if (!p) p = strstr(repo_root, "\\bin");
        if (p) *p = '\0';
    }

    char install_dir[MAX_PATH];
    if (custom_dest && strlen(custom_dest) > 0) {
        snprintf(install_dir, sizeof(install_dir), "%s", custom_dest);
    } else {
        char local_app_data[MAX_PATH];
        if (GetEnvironmentVariableA("LOCALAPPDATA", local_app_data, MAX_PATH) == 0) {
            GetEnvironmentVariableA("USERPROFILE", local_app_data, MAX_PATH);
            snprintf(install_dir, sizeof(install_dir), "%s\\.skylang", local_app_data);
        } else {
            snprintf(install_dir, sizeof(install_dir), "%s\\Skylang", local_app_data);
        }
    }

    if (!silent) {
        set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("[*] Target Installation Directory:\n    %s\n\n", install_dir);
        reset_color();
    }

    if (!check_gcc_available()) {
        set_color(FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("[WARNING] GCC compiler was not found in your system PATH!\n");
        reset_color();
        printf("Skylang requires a C11 compiler (MinGW-w64, MSYS2, or WinLibs) to compile programs.\n");
        printf("Recommended: Install MinGW-w64 from https://winlibs.com/ or https://www.msys2.org/\n\n");
        if (!silent) {
            printf("Press Enter to continue installation anyway, or Ctrl+C to cancel...");
            getchar();
        }
    } else {
        if (!silent) {
            set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            printf("[+] GCC compiler detected in PATH.\n");
            reset_color();
        }
    }

    if (!silent) printf("[*] Creating installation directories...\n");
    char bin_dir[MAX_PATH], inc_dir[MAX_PATH], src_dest_dir[MAX_PATH], lib_dir[MAX_PATH];
    snprintf(bin_dir, sizeof(bin_dir), "%s\\bin", install_dir);
    snprintf(inc_dir, sizeof(inc_dir), "%s\\include", install_dir);
    snprintf(src_dest_dir, sizeof(src_dest_dir), "%s\\src", install_dir);
    snprintf(lib_dir, sizeof(lib_dir), "%s\\lib", install_dir);

    create_directory_recursive(bin_dir);
    create_directory_recursive(inc_dir);
    create_directory_recursive(src_dest_dir);
    create_directory_recursive(lib_dir);

    char src_include[MAX_PATH], src_source[MAX_PATH];
    snprintf(src_include, sizeof(src_include), "%s\\include", repo_root);
    snprintf(src_source, sizeof(src_source), "%s\\src", repo_root);

    if (!silent) printf("[*] Copying core headers and standard library...\n");
    copy_dir_recursive(src_include, inc_dir);
    copy_dir_recursive(src_source, src_dest_dir);

    char src_skylang_exe[MAX_PATH];
    snprintf(src_skylang_exe, sizeof(src_skylang_exe), "%s\\bin\\skylang.exe", repo_root);

    char dst_skylang_exe[MAX_PATH];
    snprintf(dst_skylang_exe, sizeof(dst_skylang_exe), "%s\\skylang.exe", bin_dir);

    char dst_sky_exe[MAX_PATH];
    snprintf(dst_sky_exe, sizeof(dst_sky_exe), "%s\\sky.exe", bin_dir);

    if (GetFileAttributesA(src_skylang_exe) != INVALID_FILE_ATTRIBUTES) {
        copy_file_force(src_skylang_exe, dst_skylang_exe);
        copy_file_force(src_skylang_exe, dst_sky_exe);
    } else {
        if (!silent) printf("[*] Compiling Skylang native toolchain from source with GCC...\n");
        char build_cmd[8192];
        snprintf(build_cmd, sizeof(build_cmd),
                 "gcc -Wall -Wextra -O2 -I\"%s\" "
                 "\"%s\\main.c\" \"%s\\codegen.c\" \"%s\\runtime.c\" \"%s\\sky_stdlib.c\" "
                 "\"%s\\lexer.c\" \"%s\\parser.c\" \"%s\\compiler.c\" \"%s\\vm.c\" "
                 "\"%s\\sky_python.c\" \"%s\\sky_js.c\" \"%s\\sky_cpp.c\" \"%s\\sky_java.c\" "
                 "\"%s\\sky_go.c\" \"%s\\sky_rust.c\" "
                 "-lgc -lpython3 -lm -o \"%s\" >nul 2>&1",
                 inc_dir, src_dest_dir, src_dest_dir, src_dest_dir, src_dest_dir,
                 src_dest_dir, src_dest_dir, src_dest_dir, src_dest_dir,
                 src_dest_dir, src_dest_dir, src_dest_dir, src_dest_dir,
                 src_dest_dir, src_dest_dir, dst_skylang_exe);

        int res = system(build_cmd);
        if (res == 0 && GetFileAttributesA(dst_skylang_exe) != INVALID_FILE_ATTRIBUTES) {
            copy_file_force(dst_skylang_exe, dst_sky_exe);
        }
    }

    char src_lib[MAX_PATH], dst_lib[MAX_PATH];
    snprintf(src_lib, sizeof(src_lib), "%s\\lib\\libskylang_rt.a", repo_root);
    snprintf(dst_lib, sizeof(dst_lib), "%s\\libskylang_rt.a", lib_dir);
    if (GetFileAttributesA(src_lib) != INVALID_FILE_ATTRIBUTES) {
        copy_file_force(src_lib, dst_lib);
    }

    char sky_cmd[MAX_PATH], skylang_cmd[MAX_PATH];
    snprintf(sky_cmd, sizeof(sky_cmd), "%s\\sky.cmd", bin_dir);
    snprintf(skylang_cmd, sizeof(skylang_cmd), "%s\\skylang.cmd", bin_dir);

    FILE* fc = fopen(sky_cmd, "w");
    if (fc) {
        fputs("@echo off\r\n\"%~dp0sky.exe\" %*\r\n", fc);
        fclose(fc);
    }
    fc = fopen(skylang_cmd, "w");
    if (fc) {
        fputs("@echo off\r\n\"%~dp0skylang.exe\" %*\r\n", fc);
        fclose(fc);
    }

    char doc_src[MAX_PATH], doc_dst[MAX_PATH];
    snprintf(doc_src, sizeof(doc_src), "%s\\README.md", repo_root);
    snprintf(doc_dst, sizeof(doc_dst), "%s\\README.md", install_dir);
    copy_file_force(doc_src, doc_dst);

    snprintf(doc_src, sizeof(doc_src), "%s\\LICENSE", repo_root);
    snprintf(doc_dst, sizeof(doc_dst), "%s\\LICENSE", install_dir);
    copy_file_force(doc_src, doc_dst);

    char uninstaller_exe[MAX_PATH];
    snprintf(uninstaller_exe, sizeof(uninstaller_exe), "%s\\uninstall.exe", install_dir);
    copy_file_force(exe_path, uninstaller_exe);

    if (!silent) printf("[*] Adding %s to User Environment PATH...\n", bin_dir);
    add_to_user_path(bin_dir);

    if (!silent) printf("[*] Registering .sky and .skylang file associations...\n");
    register_file_associations(dst_sky_exe);

    register_uninstall_entry(install_dir, uninstaller_exe);

    if (!silent) {
        set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("\n======================================================================\n");
        printf("   Skylang has been successfully installed on Windows!\n");
        printf("======================================================================\n\n");
        reset_color();
        printf("Location:      %s\n", install_dir);
        printf("Binaries:      %s\n", bin_dir);
        printf("Uninstaller:   %s\n\n", uninstaller_exe);
        printf("Quick Start:\n");
        printf("  1. Open a new Command Prompt or PowerShell window.\n");
        printf("  2. Run 'sky --help' or 'sky run <file.sky>'\n\n");
        printf("Press Enter to exit installer...");
        getchar();
    }

    return 0;
}

static int run_uninstaller(bool silent) {
    if (!silent) {
        set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("======================================================================\n");
        printf("          Skylang Toolchain Windows Uninstaller\n");
        printf("======================================================================\n\n");
        reset_color();
        printf("This will completely remove Skylang from your system.\n");
        printf("Are you sure you want to proceed? (y/N): ");
        char ans = (char)getchar();
        if (ans != 'y' && ans != 'Y') {
            printf("Uninstallation cancelled.\n");
            return 0;
        }
    }

    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char install_dir[MAX_PATH];
    snprintf(install_dir, sizeof(install_dir), "%s", exe_path);
    char* last_slash = strrchr(install_dir, '\\');
    if (last_slash) *last_slash = '\0';

    char bin_dir[MAX_PATH];
    snprintf(bin_dir, sizeof(bin_dir), "%s\\bin", install_dir);

    if (!silent) printf("[*] Removing Skylang from User PATH...\n");
    remove_from_user_path(bin_dir);

    if (!silent) printf("[*] Unregistering file associations...\n");
    unregister_file_associations();

    if (!silent) printf("[*] Removing Add/Remove Programs registry entries...\n");
    unregister_uninstall_entry();

    if (!silent) {
        set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("\n[+] Skylang has been successfully uninstalled from your computer.\n");
        reset_color();
        printf("Press Enter to finish...");
        getchar();
    }

    char self_del_bat[MAX_PATH];
    char tmp[MAX_PATH];
    GetEnvironmentVariableA("TEMP", tmp, MAX_PATH);
    snprintf(self_del_bat, sizeof(self_del_bat), "%s\\skylang_cleanup_%d.bat", tmp, (int)GetCurrentProcessId());

    FILE* fb = fopen(self_del_bat, "w");
    if (fb) {
        fprintf(fb, "@echo off\r\n");
        fprintf(fb, ":repeat\r\n");
        fprintf(fb, "del /f /q \"%s\" >nul 2>&1\r\n", exe_path);
        fprintf(fb, "if exist \"%s\" goto repeat\r\n", exe_path);
        fprintf(fb, "rd /s /q \"%s\" >nul 2>&1\r\n", install_dir);
        fprintf(fb, "del /f /q \"%%~f0\" >nul 2>&1\r\n");
        fclose(fb);

        char start_cmd[MAX_PATH + 32];
        snprintf(start_cmd, sizeof(start_cmd), "cmd.exe /c start /b \"\" \"%s\"", self_del_bat);
        WinExec(start_cmd, SW_HIDE);
    }

    return 0;
}

int main(int argc, char** argv) {
    bool silent = false;
    bool is_uninstall = false;
    const char* custom_dest = NULL;

    char exe_name[MAX_PATH];
    GetModuleFileNameA(NULL, exe_name, MAX_PATH);
    if (strstr(exe_name, "uninstall.exe") != NULL || strstr(exe_name, "delete.exe") != NULL) {
        is_uninstall = true;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--silent") == 0 || strcmp(argv[i], "-s") == 0) {
            silent = true;
        } else if (strcmp(argv[i], "--uninstall") == 0 || strcmp(argv[i], "-u") == 0) {
            is_uninstall = true;
        } else if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            custom_dest = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_banner();
            printf("Usage: skylang-installer.exe [options]\n\n");
            printf("Options:\n");
            printf("  -s, --silent       Perform silent unattended installation\n");
            printf("  --dir <path>       Specify custom destination directory\n");
            printf("  -u, --uninstall    Run uninstaller\n");
            printf("  -h, --help         Show this help message\n");
            return 0;
        }
    }

    if (is_uninstall) {
        return run_uninstaller(silent);
    } else {
        return run_installer(custom_dest, silent);
    }
}
#else
int main(void) {
    printf("Skylang Windows Installer is designed for Windows.\n");
    return 0;
}
#endif
