
#ifndef _GNU_SOURCE
    // Needed for v/asprintf() and dlinfo().
    #define _GNU_SOURCE
#endif

#include <assert.h>
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


#ifndef LAUNCHER_EXE_PATH
    #error "LAUNCHER_EXE_PATH is not defined"
#endif

#ifndef LAUNCHER_LIB_DIR_PATH
    #error "LAUNCHER_LIB_DIR_PATH is not defined"
#endif


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

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
}


__attribute__((format(printf, 1, 2)))
static char* asPrintf(const char* fmt, ...)
{
    char* result;

    va_list args;
    va_start(args, fmt);

    if (vasprintf(&result, fmt, args) == -1) {
        logMsg(logError, "vasprintf(&, \"%s\", ...) failed", fmt);
        exit(EXIT_FAILURE);
    }

    va_end(args);

    return result;
}


// This type should be treated as opaque.
typedef struct ErrorCtx {
    char* text;
} ErrorCtx;


static ErrorCtx* errorCtxCreate(void)
{
    ErrorCtx* ctx = calloc(1, sizeof(ErrorCtx));
    if (!ctx) {
        logMsg(logError, "calloc for ErrorCtx: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ctx;
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

    // The existing error text can be passed as a format argument, so
    // don't free it until we create the new one.
    char* newText;

    va_list args;
    va_start(args, fmt);

    if (vasprintf(&newText, fmt, args) == -1) {
        logMsg(
            logError,
            "errorCtxSetText vasprintf(&, \"%s\", ...) failed",
            fmt);
        exit(EXIT_FAILURE);
    }

    va_end(args);

    if (ctx->text)
        free(ctx->text);

    ctx->text = newText;
}


static const char* getBaseName(const char* path)
{
    const char* lastSlash = strrchr(path, '/');
    return lastSlash ? lastSlash + 1 : path;
}


static char* getRealPath(const char* path, ErrorCtx* errorCtx)
{
    char* result = realpath(path, NULL);
    if (!result)
        errorCtxSetText(
            errorCtx,
            "realpath(\"%s\", NULL): %s",
            path,
            strerror(errno));

    return result;
}


static const char* const ldLibraryPathEnv = "LD_LIBRARY_PATH";


static bool prependToColonSeparatedEnv(
    const char* envVar, const char* val, ErrorCtx* errorCtx)
{
    const char* oldVal = getenv(envVar);
    if (!oldVal)
        oldVal = "";

    char* newVal = asPrintf(
        "%s%s%s", val, *oldVal ? ":" : "", oldVal);

    const bool result = setenv(envVar, newVal, 1) == 0;
    if (!result)
        errorCtxSetText(
            errorCtx,
            "setenv(\"%s\", \"%s\", 1): %s",
            envVar, newVal, strerror(errno));

    free(newVal);

    return result;
}


static char* getExeDir(ErrorCtx* errorCtx)
{
    const char* procPath = "/proc/self/exe";
    char* result = getRealPath(procPath, errorCtx);
    if (!result) {
        errorCtxSetText(
            errorCtx,
            "Can't get real path of \"%s\": %s",
            procPath, errorCtxGetText(errorCtx));
        return NULL;
    }

    char* lastSlash = strrchr(result, '/');
    if (lastSlash)
        *lastSlash = 0;

    return result;
}


static char* getSysLibPath(const char* libName, ErrorCtx* errorCtx)
{
    void* handle = dlopen(libName, RTLD_LAZY);
    if (!handle) {
        errorCtxSetText(errorCtx, "dlopen: %s", dlerror());
        return NULL;
    }

    char* result = NULL;

    struct link_map* linkMap = NULL;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &linkMap) == -1)
        errorCtxSetText(errorCtx, "dlinfo: %s", dlerror());
    else {
        // In practice, the link_map entry corresponding to the main
        // lib will be the first in the list, but we do a generic
        // search since this behavior is not documented.
        for (; linkMap; linkMap = linkMap->l_next) {
            if (strcmp(getBaseName(linkMap->l_name), libName) != 0)
                continue;

            result = getRealPath(linkMap->l_name, errorCtx);
            if (!result)
                errorCtxSetText(
                    errorCtx,
                    "Can't get real path of \"%s\": %s",
                    linkMap->l_name, errorCtxGetText(errorCtx));

            break;
        }

        if (!linkMap)
            errorCtxSetText(
                errorCtx, "Library was not found in link_map");
    }

    dlclose(handle);
    return result;
}


static char* getFallbackLibPath(
    const char* libName, const char* libDir, ErrorCtx* errorCtx)
{
    char* path = asPrintf("%s/%s", libDir, libName);

    char* result = getRealPath(path, errorCtx);
    if (!result) {
        errorCtxSetText(
            errorCtx,
            "Can't get real path of \"%s\": %s",
            path, errorCtxGetText(errorCtx));

        free(path);
        return NULL;
    }

    // The checks rated to the version string syntax will be done by
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

    free(path);

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
            break;
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
            break;
        }

        s = end;
        if (*s == '.') {
            if (i + 1 == numVersionNumbers) {
                errorCtxSetText(
                    errorCtx,
                    "All %i numbers were parsed, but \"%s\" remains",
                    numVersionNumbers, s);
                break;
            }

            ++s;
        } else if (*s) {
            errorCtxSetText(
                errorCtx,
                "Number is followed by an unexpected \"%c\" at index "
                "%zu",
                *s, s - str);
            break;
        } else
            return true;
    }

    return false;
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
            "The base name (\"%s\") doesn't start with %s",
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

    char* fallbackLibPath = getFallbackLibPath(
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

        free(fallbackLibPath);
        return false;
    }

    free(fallbackLibPath);

    // The system libraries are not under our control, so we don't
    // threat any issues related to them as errors.

    char* sysLibPath = getSysLibPath(libName, errorCtx);
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
        free(sysLibPath);
        return true;
    }

    free(sysLibPath);

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


static bool setUpFallbackLibs(
    const char* libDirPath, ErrorCtx* errorCtx)
{
    char* fallbackLibDirPath = asPrintf("%s/fallback", libDirPath);

    DIR* dirp = opendir(fallbackLibDirPath);
    if (!dirp) {
        bool result = true;

        if (errno == ENOENT)
            logMsg(
                logDebug,
                "Skipping fallback libs setup as the \"%s\" "
                "directory does not exist",
                fallbackLibDirPath);
        else {
            errorCtxSetText(
                errorCtx,
                "opendir(\"%s\"): %s",
                fallbackLibDirPath, strerror(errno));

            result = false;
        }

        free(fallbackLibDirPath);
        return result;
    }

    bool result = true;

    while (true) {
        errno = 0;
        struct dirent* dp = readdir(dirp);
        if (!dp) {
            if (errno != 0) {
                errorCtxSetText(
                    errorCtx,
                    "readdir() for \"%s\" handle: %s",
                    fallbackLibDirPath, strerror(errno));
                result = false;
            }

            break;
        }

        if (strcmp(dp->d_name, ".") == 0
                || strcmp(dp->d_name, "..") == 0)
            continue;

        char* curLibFallbackDirPath = asPrintf(
            "%s/%s", fallbackLibDirPath, dp->d_name);

        result = setUpFallbackLib(
            dp->d_name, curLibFallbackDirPath, errorCtx);

        free(curLibFallbackDirPath);

        if (!result) {
            errorCtxSetText(
                errorCtx,
                "Can't set up %s from \"%s\": %s",
                dp->d_name,
                curLibFallbackDirPath,
                errorCtxGetText(errorCtx));
            break;
        }
    }

    closedir(dirp);
    free(fallbackLibDirPath);

    return result;
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

    ErrorCtx* errorCtx = errorCtxCreate();

    char* launcherDir = getExeDir(errorCtx);
    if (!launcherDir) {
        logMsg(
            logError,
            "Can't get launcher dir: %s",
            errorCtxGetText(errorCtx));

        errorCtxDelete(errorCtx);
        return EXIT_FAILURE;
    }

    char* exePath = asPrintf(
        "%s/%s", launcherDir, LAUNCHER_EXE_PATH);
    char* libDirPath = asPrintf(
        "%s/%s", launcherDir, LAUNCHER_LIB_DIR_PATH);

    free(launcherDir);

    logMsg(logDebug, "Exe path: %s", exePath);
    logMsg(logDebug, "Lib dir path: %s", libDirPath);

    if (!setUpLibs(libDirPath, errorCtx)) {
        errorCtxSetText(
            errorCtx,
            "Can't set up libraries from \"%s\": %s",
            libDirPath,
            errorCtxGetText(errorCtx));

        free(exePath);
        free(libDirPath);
        errorCtxDelete(errorCtx);
        return EXIT_FAILURE;
    }

    const char* ldLibraryPathEnvVar = getenv(ldLibraryPathEnv);
    if (ldLibraryPathEnvVar)
        logMsg(
            logDebug,
            "%s=\"%s\"",
            ldLibraryPathEnv, ldLibraryPathEnvVar);

    if (enableDebug) {
        // Show the ldd output so that users can include it in bug
        // reports. It will be non-trivial for the user to do this
        // manually if the application is packaged as AppImage.
        logMsg(logDebug, "ldd output:");
        if (!callLdd(exePath, errorCtx))
            logMsg(
                logDebug,
                "Can't execute ldd: %s",
                errorCtxGetText(errorCtx));
    }

    free(libDirPath);
    errorCtxDelete(errorCtx);

    *argv = exePath;
    execvp(*argv, argv);
    logMsg(
        logError,
        "execvp(\"%s\", ...): %s",
        *argv, strerror(errno));

    free(exePath);

    return EXIT_FAILURE;
}