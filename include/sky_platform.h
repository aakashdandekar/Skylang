#ifndef SKY_PLATFORM_H
#define SKY_PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)
    #define SKY_OS_WINDOWS 1
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #include <process.h>

    #define SKY_EXE_EXT ".exe"
    #define SKY_SO_EXT ".dll"
    #define SKY_PATH_SEP '\\'
    #define SKY_PATH_SEP_STR "\\"
    #define SKY_DEV_NULL "nul"

    #ifndef F_OK
    #define F_OK 0
    #endif
    #ifndef R_OK
    #define R_OK 4
    #endif
    #ifndef W_OK
    #define W_OK 2
    #endif

    #define sky_access _access
    #define sky_getpid _getpid
    #define sky_unlink _unlink

#elif defined(__APPLE__) && defined(__MACH__)
    #define SKY_OS_MACOS 1
    #include <mach-o/dyld.h>
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/wait.h>

    #define SKY_EXE_EXT ""
    #define SKY_SO_EXT ".dylib"
    #define SKY_PATH_SEP '/'
    #define SKY_PATH_SEP_STR "/"
    #define SKY_DEV_NULL "/dev/null"

    #define sky_access access
    #define sky_getpid getpid
    #define sky_unlink unlink

#else
    #define SKY_OS_LINUX 1
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/wait.h>

    #define SKY_EXE_EXT ""
    #define SKY_SO_EXT ".so"
    #define SKY_PATH_SEP '/'
    #define SKY_PATH_SEP_STR "/"
    #define SKY_DEV_NULL "/dev/null"

    #define sky_access access
    #define sky_getpid getpid
    #define sky_unlink unlink
#endif

/* Dynamic library loading wrappers */
static inline void* sky_dlopen(const char* path) {
#if defined(SKY_OS_WINDOWS)
    return (void*)LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
}

static inline void* sky_dlsym(void* handle, const char* symbol) {
#if defined(SKY_OS_WINDOWS)
    return (void*)GetProcAddress((HMODULE)handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}

static inline int sky_dlclose(void* handle) {
#if defined(SKY_OS_WINDOWS)
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
#else
    return dlclose(handle);
#endif
}

static inline const char* sky_dlerror(void) {
#if defined(SKY_OS_WINDOWS)
    static char err_buf[256];
    DWORD err = GetLastError();
    if (err == 0) return "Unknown error";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   err_buf, sizeof(err_buf), NULL);
    size_t len = strlen(err_buf);
    while (len > 0 && (err_buf[len - 1] == '\r' || err_buf[len - 1] == '\n' || err_buf[len - 1] == ' ')) {
        err_buf[--len] = '\0';
    }
    return err_buf;
#else
    const char* err = dlerror();
    return err ? err : "Unknown error";
#endif
}

/* Temp directory locator */
static inline void sky_get_temp_dir(char* out, size_t max_len) {
#if defined(SKY_OS_WINDOWS)
    DWORD len = GetTempPathA((DWORD)max_len, out);
    if (len > 0 && len < max_len) {
        if (out[len - 1] == '\\' || out[len - 1] == '/') {
            out[len - 1] = '\0';
        }
        return;
    }
    const char* tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "C:\\Windows\\Temp";
    snprintf(out, max_len, "%s", tmp);
#else
    const char* tmp = getenv("TMPDIR");
    if (!tmp || tmp[0] == '\0') tmp = "/tmp";
    snprintf(out, max_len, "%s", tmp);
#endif
}

/* Executable path discovery */
static inline bool sky_get_executable_path(char* out, size_t max_len) {
#if defined(SKY_OS_WINDOWS)
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)max_len);
    if (len > 0 && len < max_len) {
        out[len] = '\0';
        return true;
    }
    return false;
#elif defined(SKY_OS_MACOS)
    uint32_t size = (uint32_t)max_len;
    if (_NSGetExecutablePath(out, &size) == 0) {
        return true;
    }
    return false;
#elif defined(SKY_OS_LINUX)
    ssize_t len = readlink("/proc/self/exe", out, max_len - 1);
    if (len != -1) {
        out[len] = '\0';
        return true;
    }
    return false;
#else
    return false;
#endif
}

/* Directory extraction from file path */
static inline void sky_extract_dirname(const char* path, char* out, size_t max_len) {
    if (!path || !path[0]) {
        snprintf(out, max_len, ".");
        return;
    }
    snprintf(out, max_len, "%s", path);
    char* last_slash = strrchr(out, '/');
    char* last_backslash = strrchr(out, '\\');
    char* sep = (last_backslash > last_slash) ? last_backslash : last_slash;
    if (sep) {
        if (sep == out) {
            *(sep + 1) = '\0';
        } else {
            *sep = '\0';
        }
    } else {
        snprintf(out, max_len, ".");
    }
}

#endif /* SKY_PLATFORM_H */
