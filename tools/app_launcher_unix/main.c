
#ifndef _GNU_SOURCE
    // Required for dlinfo() and some utilities like vasprintf().
    #define _GNU_SOURCE
#endif

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <link.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


static void cleanupStr(char** ctx)
{
    free(*ctx);
}
#define CLEANUP_STR __attribute__((cleanup(cleanupStr)))


static void cleanupVaList(va_list* va)
{
    va_end(*va);
}
#define CLEANUP_VA_LIST __attribute__((cleanup(cleanupVaList)))


static bool enableDebug = false;


typedef enum {
    logDebug,
    logError
} LogSeverity;


__attribute__((format(printf, 2, 3)))
static void logMsg(LogSeverity severity, const char* fmt, ...)
{
    if (severity == logDebug && !enableDebug)
        return;

    fputs("[LAUNCHER ", stderr);

    switch (severity) {
    case logDebug:
        fputs("DEBUG", stderr);
        break;
    case logError:
        fputs("ERROR", stderr);
        break;
    }

    fputs("] ", stderr);

    CLEANUP_VA_LIST va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);

    fputc('\n', stderr);
}


static char* xStrdup(const char* s)
{
    char* result = strdup(s);
    if (result)
        return result;

    logMsg(logError, "strdup(): %s", strerror(errno));
    exit(EXIT_FAILURE);
}


static char* xStrndup(const char* s, size_t len)
{
    char* result = strndup(s, len);
    if (result)
        return result;

    logMsg(logError, "strndup(): %s", strerror(errno));
    exit(EXIT_FAILURE);
}


static char* xVasprintf(const char *fmt, va_list args)
{
    char* result;
    if (vasprintf(&result, fmt, args) != -1)
        return result;

    logMsg(logError, "vasprintf(&, \"%s\", ...) failed", fmt);
    exit(EXIT_FAILURE);
}


__attribute__((format(printf, 1, 2)))
static char* xAsprintf(const char* fmt, ...)
{
    CLEANUP_VA_LIST va_list args;
    va_start(args, fmt);

    return xVasprintf(fmt, args);
}


// This type should be treated as opaque.
typedef struct ErrorCtx {
    char* text;
} ErrorCtx;


static ErrorCtx* errorCtxCreate(void)
{
    ErrorCtx* ctx = calloc(1, sizeof(ErrorCtx));
    if (ctx)
        return ctx;

    logMsg(logError, "calloc() for ErrorCtx: %s", strerror(errno));
    exit(EXIT_FAILURE);
}


static void errorCtxDelete(ErrorCtx* ctx)
{
    if (ctx && ctx->text)
        free(ctx->text);

    free(ctx);
}


static const char* errorCtxGetText(const ErrorCtx* ctx)
{
    return (ctx && ctx->text) ? ctx->text : "";
}


__attribute__((format(printf, 2, 3)))
static void errorCtxSetText(ErrorCtx* ctx, const char* fmt, ...)
{
    if (!ctx)
        return;

    CLEANUP_VA_LIST va_list args;
    va_start(args, fmt);

    // The existing error text can be passed as a format argument, so
    // don't free it until we create the new one.
    char* newText = xVasprintf(fmt, args);

    if (ctx->text)
        free(ctx->text);

    ctx->text = newText;
}


static void cleanupErrorCtx(ErrorCtx** ctx)
{
    errorCtxDelete(*ctx);
}
#define CLEANUP_ERROR_CTX __attribute__((cleanup(cleanupErrorCtx)))


static char* getDirName(const char* path)
{
    const char* sep = NULL;
    for (const char* s = path; *s; ++s)
        if (*s == '/' && (s == path || s[-1] != '/'))
            sep = s;

    if (!sep)
        return xStrdup(".");
    if (sep == path)
        return xStrdup("/");

    return xStrndup(path, sep - path);
}


static const char* getBaseName(const char* path)
{
    const char* lastSep = strrchr(path, '/');
    return lastSep ? lastSep + 1 : path;
}


static char* getRealPath(const char* path, ErrorCtx* errorCtx)
{
    char* result = realpath(path, NULL);
    if (!result)
        errorCtxSetText(errorCtx, "realpath(): %s", strerror(errno));

    return result;
}


static char* getExePath(ErrorCtx* errorCtx)
{
    const char* procPath = "/proc/self/exe";

    char* result = getRealPath(procPath, errorCtx);
    if (result)
        return result;

    errorCtxSetText(
        errorCtx,
        "Can't get real path of \"%s\": %s",
        procPath, errorCtxGetText(errorCtx));
    return NULL;
}


static const char* const ldLibraryPathEnv = "LD_LIBRARY_PATH";


static bool prependToColonSeparatedEnv(
    const char* envVar, const char* val, ErrorCtx* errorCtx)
{
    const char* oldVal = getenv(envVar);
    if (!oldVal)
        oldVal = "";

    CLEANUP_STR char* newVal = xAsprintf(
        "%s%s%s", val, *oldVal ? ":" : "", oldVal);

    if (setenv(envVar, newVal, 1) == 0)
        return true;

    errorCtxSetText(
        errorCtx,
        "setenv(\"%s\", \"%s\", 1): %s",
        envVar, newVal, strerror(errno));
    return false;
}


static void cleanupFile(FILE** fp)
{
    if (*fp)
        fclose(*fp);
}
#define CLEANUP_FILE __attribute__((cleanup(cleanupFile)))


typedef struct LauncherPaths {
    char* exe;
    char* libDir;
} LauncherPaths;


static void cleanupLauncherPaths(LauncherPaths* paths)
{
    free(paths->exe);
    free(paths->libDir);
}
#define CLEANUP_LAUNCHER_PATHS \
    __attribute__((cleanup(cleanupLauncherPaths)))


void cfgExtractKeyValue(
    char* line, const char** key, const char** value)
{
    assert(key);
    assert(value);

    char* keyBegin = line;
    while (isspace((unsigned char)*keyBegin))
        ++keyBegin;

    char* keyEnd = keyBegin;
    while (*keyEnd && !isspace((unsigned char)*keyEnd))
        ++keyEnd;

    char* valueBegin = keyEnd;
    while (isspace((unsigned char)*valueBegin))
        ++valueBegin;

    char* valueEnd = valueBegin;
    for (char* s = valueEnd; *s; ++s)
        if (!isspace((unsigned char)*s))
            valueEnd = s + 1;

    *key = keyBegin;
    *keyEnd = 0;

    *value = valueBegin;
    *valueEnd = 0;
}


static const char* const cfgKeyExe = "exe";
static const char* const cfgKeyLibDir = "lib_dir";


static char** cfgGetFieldForKey(LauncherPaths* paths, const char* key)
{
    if (strcmp(key, cfgKeyExe) == 0)
        return &paths->exe;
    if (strcmp(key, cfgKeyLibDir) == 0)
        return &paths->libDir;

    return NULL;
}


static bool cfgProcessLine(
    char* line,
    const char* baseDirPath,
    LauncherPaths* paths,
    ErrorCtx* errorCtx)
{
    assert(*baseDirPath == '/');
    assert(paths);

    const char* key = NULL;
    const char* path = NULL;
    cfgExtractKeyValue(line, &key, &path);

    if (!*key)
        // Ignore empty lines.
        return true;

    char** dst = cfgGetFieldForKey(paths, key);
    if (!dst) {
        errorCtxSetText(errorCtx, "Unknown key \"%s\"", key);
        return false;
    }

    if (*dst) {
        errorCtxSetText(errorCtx, "Duplicate key \"%s\"", key);
        return false;
    }

    if (!*path) {
        errorCtxSetText(errorCtx, "\"%s\" path is empty", key);
        return false;
    }

    if (*path == '/') {
        errorCtxSetText(
            errorCtx, "\"%s\" path must not be absolute", key);
        return false;
    }

    CLEANUP_STR char* fullPath = xAsprintf(
        "%s/%s", baseDirPath, path);

    char* realPath = getRealPath(fullPath, errorCtx);
    if (!realPath) {
        errorCtxSetText(
            errorCtx,
            "Can't get real path of \"%s\": %s",
            fullPath, errorCtxGetText(errorCtx));
        return false;
    }

    assert(!*dst);
    *dst = realPath;

    return true;
}


static bool cfgLoad(
    const char* cfgPath, LauncherPaths* paths, ErrorCtx* errorCtx)
{
    assert(paths);
    *paths = (LauncherPaths){0};

    CLEANUP_FILE FILE* fp = fopen(cfgPath, "r");
    if (!fp) {
        errorCtxSetText(
            errorCtx, "fopen(..., \"r\"): %s", strerror(errno));
        return false;
    }

    CLEANUP_STR char* baseDirPath = getDirName(cfgPath);

    CLEANUP_STR char* line = NULL;
    size_t lineSize = 0;

    for (int lineNum = 1; true; ++lineNum) {
        const ssize_t lineLen = getline(&line, &lineSize, fp);
        if (lineLen == -1)
            break;

        if (!cfgProcessLine(line, baseDirPath, paths, errorCtx)) {
            errorCtxSetText(
                errorCtx,
                "Line %i: %s",
                lineNum, errorCtxGetText(errorCtx));
            return false;
        }
    }

    if (ferror(fp)) {
        errorCtxSetText(errorCtx, "Read error: %s", strerror(errno));
        return false;
    }

    const char* cfgKeys[] = {cfgKeyExe, cfgKeyLibDir, NULL};
    for (const char** key = cfgKeys; *key; ++key) {
        char** field = cfgGetFieldForKey(paths, *key);
        assert(field);

        if (!*field) {
            errorCtxSetText(errorCtx, "\"%s\" is not set", *key);
            return false;
        }
    }

    return true;
}


static bool loadLauncherPaths(
    LauncherPaths* paths, ErrorCtx* errorCtx)
{
    assert(paths);

    CLEANUP_STR char* exePath = getExePath(errorCtx);
    if (!exePath) {
        errorCtxSetText(
            errorCtx,
            "Can't get launcher executable path: %s",
            errorCtxGetText(errorCtx));
        return false;
    }

    CLEANUP_STR char* cfgPath = xAsprintf("%s.cfg", exePath);
    if (!cfgLoad(cfgPath, paths, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "%s: %s",
            cfgPath, errorCtxGetText(errorCtx));
        return false;
    }

    return true;
}


static void cleanupDlHandle(void** handle)
{
    if (*handle)
        dlclose(*handle);
}
#define CLEANUP_DL_HANDLE __attribute__((cleanup(cleanupDlHandle)))


static char* getSysLibPath(const char* libName, ErrorCtx* errorCtx)
{
    CLEANUP_DL_HANDLE void* handle = dlopen(libName, RTLD_LAZY);
    if (!handle) {
        errorCtxSetText(errorCtx, "dlopen(): %s", dlerror());
        return NULL;
    }

    struct link_map* linkMap = NULL;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &linkMap) == -1) {
        errorCtxSetText(
            errorCtx, "dlinfo(RTLD_DI_LINKMAP): %s", dlerror());
        return NULL;
    }

    // In practice, the link_map entry corresponding to the main lib
    // will be the first in the list, but we do a generic search since
    // this behavior is not documented.
    for (; linkMap; linkMap = linkMap->l_next) {
        if (strcmp(getBaseName(linkMap->l_name), libName) != 0)
            continue;

        char* path = getRealPath(linkMap->l_name, errorCtx);
        if (path)
            return path;

        errorCtxSetText(
            errorCtx,
            "Can't get real path of \"%s\": %s",
            linkMap->l_name, errorCtxGetText(errorCtx));
        return NULL;
    }

    errorCtxSetText(errorCtx, "Library was not found in link_map");
    return NULL;
}


static char* getFallbackLibPath(
    const char* libName, const char* libDir, ErrorCtx* errorCtx)
{
    CLEANUP_STR char* path = xAsprintf("%s/%s", libDir, libName);

    char* result = getRealPath(path, errorCtx);
    if (!result) {
        errorCtxSetText(
            errorCtx,
            "Can't get real path of \"%s\": %s",
            path, errorCtxGetText(errorCtx));
        return NULL;
    }

    // The checks related to the version string syntax will be done by
    // parseVersion. Here we only check the symbolic link resolution.
    const size_t pathLen = strlen(path);
    if (strncmp(result, path, pathLen) != 0
            || !result[pathLen]
            || strchr(result + pathLen, '/')) {
        errorCtxSetText(
            errorCtx,
            "\"%s\" must be a symbolic link pointing to another file "
            "inside the same directory, with extra version numbers "
            "in the name, e.g., libstdc++.so.6 pointing to "
            "libstdc++.so.6.0.28",
            path);

        free(result);
        result = NULL;
    }

    return result;
}


enum {
    // Library names normally end with 3 numbers. Since the first one
    // designates completely ABI-incompatible versions, we don't
    // consider it during the version comparison (the linker and
    // dlopen() do the same). Thus, we need storage for 2 numbers, but
    // let's add more in case of some weird systems.
    numVersionNumbers = 6
};


typedef struct VersionInfo {
    int numbers[numVersionNumbers];
} VersionInfo;


static int cmpVersion(const VersionInfo* a, const VersionInfo* b)
{
    assert(a);
    assert(b);

    for (int i = 0; i < numVersionNumbers; ++i)
        if (a->numbers[i] < b->numbers[i])
            return -1;
        else if (a->numbers[i] > b->numbers[i])
            return 1;

    return 0;
}


static bool parseVersion(
    const char* str, VersionInfo* version, ErrorCtx* errorCtx)
{
    assert(version);
    *version = (VersionInfo){0};

    const char* s = str;
    for (int i = 0; i < numVersionNumbers; ++i) {
        // We don't need optional spaces, -, and + accepted by strtol.
        if (*s < '0' || *s > '9') {
            errorCtxSetText(
                errorCtx,
                "Expected a number, but got \"%s\" at index %zu",
                s, s - str);
            return false;
        }

        char* end;
        version->numbers[i] = strtol(s, &end, 10);
        if (end == s) {
            // Should not actually happen due to the first char check
            // above.
            errorCtxSetText(
                errorCtx,
                "Invalid number \"%s\" at index %zu",
                s, s - str);
            return false;
        }

        s = end;
        if (*s == '.') {
            if (i + 1 == numVersionNumbers) {
                errorCtxSetText(
                    errorCtx,
                    "All %i numbers were parsed, but \"%s\" remains",
                    numVersionNumbers, s);
                return false;
            }

            ++s;
        } else if (*s) {
            errorCtxSetText(
                errorCtx,
                "Number is followed by an unexpected \"%c\" at index "
                "%zu",
                *s, s - str);
            return false;
        } else
            return true;
    }

    return true;
}


static bool getLibVersion(
    const char* libName,
    const char* libPath,
    VersionInfo* version,
    ErrorCtx* errorCtx)
{
    assert(version);

    const size_t libNameLen = strlen(libName);

    const char* baseName = getBaseName(libPath);
    if (strncmp(baseName, libName, libNameLen) != 0) {
        errorCtxSetText(
            errorCtx,
            "The base name (\"%s\") doesn't start with \"%s\"",
            baseName, libName);
        return false;
    }

    if (baseName[libNameLen] != '.') {
        errorCtxSetText(
            errorCtx,
            "No extra version numbers in \"%s\". The name is "
            "expected to end with a version like \".0.1\" rather "
            "than \"%s\".",
            baseName, baseName + libNameLen);
        return false;
    }

    const char* versionStr = baseName + libNameLen + 1;

    if (!parseVersion(versionStr, version, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "\"%s\" ends with an invalid version string \"%s\": %s",
            baseName, versionStr, errorCtxGetText(errorCtx));
        return false;
    }

    return true;
}


typedef enum {
    libSelectionSystem,
    libSelectionFallback
} LibSelection;


static bool selectLib(
    const char* libName,
    const char* fallbackLibDir,
    LibSelection* selectedLib,
    ErrorCtx* errorCtx)
{
    assert(selectedLib);

    logMsg(
        logDebug,
        "Selecting between system and fallback %s",
        libName);

    // First handle everything related to the fallback lib to detect
    // packaging errors early.

    CLEANUP_STR char* fallbackLibPath = getFallbackLibPath(
        libName, fallbackLibDir, errorCtx);
    if (!fallbackLibPath) {
        errorCtxSetText(
            errorCtx,
            "Can't get path of fallback %s in \"%s\": %s",
            libName, fallbackLibDir, errorCtxGetText(errorCtx));
        return false;
    }

    logMsg(
        logDebug,
        "Fallback %s is in \"%s\"",
        libName, fallbackLibPath);

    VersionInfo fallbackLibVer;
    if (!getLibVersion(
            libName, fallbackLibPath, &fallbackLibVer, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't extract the version of fallback %s from the path "
            "\"%s\": %s",
            libName, fallbackLibPath, errorCtxGetText(errorCtx));
        return false;
    }

    // The system libraries are not under our control, so we don't
    // threat any issues related to them as errors.

    CLEANUP_STR char* sysLibPath = getSysLibPath(libName, errorCtx);
    if (!sysLibPath) {
        logMsg(
            logDebug,
            "Can't get path of system %s: %s. Selecting the fallback "
            "library.",
            libName, errorCtxGetText(errorCtx));

        *selectedLib = libSelectionFallback;
        return true;
    }

    logMsg(logDebug, "System %s is in \"%s\"", libName, sysLibPath);

    VersionInfo sysLibVer;
    if (!getLibVersion(libName, sysLibPath, &sysLibVer, errorCtx)) {
        // Since the linker was able to resolve the system library
        // name, we can safely assume that its version >= the fallback
        // library: after all, the fallback library should normally
        // come from the oldest system setup supported by the given
        // application package, and it's not our fault if the user
        // tries to run it on an older system.
        logMsg(
            logDebug,
            "Can't extract the version of system %s from the path "
            "\"%s\" (%s). Selecting it anyway, assuming that its "
            "version is not older than the version of the fallback.",
            libName, sysLibPath, errorCtxGetText(errorCtx));

        *selectedLib = libSelectionSystem;
        return true;
    }

    *selectedLib =
        (cmpVersion(&sysLibVer, &fallbackLibVer) >= 0)
            ? libSelectionSystem
            : libSelectionFallback;

    logMsg(
        logDebug,
        "%s %s is selected",
        *selectedLib == libSelectionSystem ? "System" : "Fallback",
        libName);

    return true;
}


static bool setUpFallbackLib(
    const char* libName,
    const char* fallbackLibDir,
    ErrorCtx* errorCtx)
{
    LibSelection selectedLib;
    if (!selectLib(libName, fallbackLibDir, &selectedLib, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't select %s: %s",
            libName, errorCtxGetText(errorCtx));
        return false;
    }

    if (selectedLib == libSelectionFallback
            && !prependToColonSeparatedEnv(
                ldLibraryPathEnv, fallbackLibDir, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't prepend \"%s\" to the %s environment variable: %s",
            fallbackLibDir,
            ldLibraryPathEnv,
            errorCtxGetText(errorCtx));
        return false;
    }

    return true;
}


static void cleanupDir(DIR** dir)
{
    if (*dir)
        closedir(*dir);
}
#define CLEANUP_DIR __attribute__((cleanup(cleanupDir)))


static bool setUpFallbackLibs(
    const char* libDirPath, ErrorCtx* errorCtx)
{
    CLEANUP_STR char* fallbackLibDirPath = xAsprintf(
        "%s/fallback", libDirPath);

    CLEANUP_DIR DIR* dirp = opendir(fallbackLibDirPath);
    if (!dirp) {
        if (errno == ENOENT) {
            logMsg(
                logDebug,
                "Skipping fallback libs setup as the \"%s\" "
                "directory does not exist",
                fallbackLibDirPath);
            return true;
        }

        errorCtxSetText(
            errorCtx,
            "opendir(\"%s\"): %s",
            fallbackLibDirPath, strerror(errno));
        return false;
    }

    while (true) {
        errno = 0;
        struct dirent* dp = readdir(dirp);
        if (!dp) {
            if (errno == 0)
                break;

            errorCtxSetText(
                errorCtx,
                "readdir() for \"%s\" handle: %s",
                fallbackLibDirPath, strerror(errno));
            return false;
        }

        if (strcmp(dp->d_name, ".") == 0
                || strcmp(dp->d_name, "..") == 0)
            continue;

        CLEANUP_STR char* curLibFallbackDirPath = xAsprintf(
            "%s/%s", fallbackLibDirPath, dp->d_name);

        if (!setUpFallbackLib(
                dp->d_name, curLibFallbackDirPath, errorCtx)) {
            errorCtxSetText(
                errorCtx,
                "Can't set up %s from \"%s\": %s",
                dp->d_name,
                curLibFallbackDirPath,
                errorCtxGetText(errorCtx));
            return false;
        }
    }

    return true;
}


static bool setUpLibs(const char* libDirPath, ErrorCtx* errorCtx)
{
    if (!prependToColonSeparatedEnv(
            ldLibraryPathEnv, libDirPath, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't prepend \"%s\" to the %s environment variable: %s",
            libDirPath,
            ldLibraryPathEnv,
            errorCtxGetText(errorCtx));
        return false;
    }

    if (!setUpFallbackLibs(libDirPath, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't set up fallback libraries: %s",
            errorCtxGetText(errorCtx));
        return false;
    }

    return true;
}


static bool callLdd(const char* exePath, ErrorCtx* errorCtx)
{
    const pid_t pid = fork();
    if (pid == -1) {
        errorCtxSetText(errorCtx, "fork(): %s", strerror(errno));
        return false;
    }

    if (pid == 0) {
        const char* args[] = {"ldd", exePath, NULL};
        execvp(args[0], (char* const*)args);
        _Exit(EXIT_FAILURE);
    }

    if (waitpid(pid, NULL, 0) == -1) {
        errorCtxSetText(
            errorCtx, "waitpid() for ldd: %s", strerror(errno));
        return false;
    }

    return true;
}


int main(int argc, char* argv[])
{
    (void)argc;

    const char* launcherDebugEnvVar = getenv("LAUNCHER_DEBUG");
    if (launcherDebugEnvVar
            && *launcherDebugEnvVar
            && strcmp(launcherDebugEnvVar, "0") != 0)
        enableDebug = true;

    CLEANUP_ERROR_CTX ErrorCtx* errorCtx = errorCtxCreate();

    CLEANUP_LAUNCHER_PATHS LauncherPaths launcherPaths = {0};
    if (!loadLauncherPaths(&launcherPaths, errorCtx)) {
        logMsg(
            logError,
            "Can't load launcher paths: %s",
            errorCtxGetText(errorCtx));
        return EXIT_FAILURE;
    }

    logMsg(logDebug, "Exe path: %s", launcherPaths.exe);
    logMsg(logDebug, "Lib dir path: %s", launcherPaths.libDir);

    if (!setUpLibs(launcherPaths.libDir, errorCtx)) {
        logMsg(
            logError,
            "Can't set up libraries from \"%s\": %s",
            launcherPaths.libDir,
            errorCtxGetText(errorCtx));
        return EXIT_FAILURE;
    }

    const char* ldLibraryPathEnvVar = getenv(ldLibraryPathEnv);
    if (ldLibraryPathEnvVar)
        logMsg(
            logDebug, "%s=%s", ldLibraryPathEnv, ldLibraryPathEnvVar);

    if (enableDebug) {
        // It's easy to run ldd manually (we've shown LD_LIBRARY_PATH
        // above), but we don't want to ask users to take extra steps
        // when reporting bugs.
        logMsg(logDebug, "ldd output:");
        if (!callLdd(launcherPaths.exe, errorCtx))
            logMsg(
                logDebug,
                "Can't execute ldd: %s",
                errorCtxGetText(errorCtx));
    }

    *argv = launcherPaths.exe;
    execv(*argv, argv);
    logMsg(
        logError, "execv(\"%s\", ...): %s", *argv, strerror(errno));

    return EXIT_FAILURE;
}
