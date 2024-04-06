
#include "prepare_environment.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dpso_utils/os.h"


// On some hardware, OpenMP multithreading in Tesseract 4 and 5
// results in dramatically slow recognition. The Tesseract developers
// recommend to either build the library with OpenMP disabled, or to
// set the OMP_THREAD_LIMIT environment variable to 1 before running
// the program that use Tesseract.
//
// Although the Tesseract developers are going to disable OpenMP in
// Autotools builds by default [1], this is not yet done as of version
// 5.2.0. As such, most of Unix-like systems ship the library with
// OpenMP enabled, and OMP_THREAD_LIMIT is the only way for us to fix
// the issue. For other platforms, we build Tesseract without OpenMP
// ourselves, and this hack is unnecessary.
//
// We can set OMP_THREAD_LIMIT from a launcher script like:
//
//   #!/bin/sh
//   OMP_THREAD_LIMIT=1 exec our-program "$@"
//
// However, two executables (the script and the actual binary with
// some special name, e.g., ending with a "-bin" suffix) may confuse
// users running programs from the command line. Instead, we set the
// variable and restart the program directly from the executable. As
// such, uiPrepareEnvironment() should be the first call in main().
//
// [1] https://github.com/tesseract-ocr/tesstrain/issues/259


void uiPrepareEnvironment(char* argv[])
{
    const auto* ompThreadLimit = "OMP_THREAD_LIMIT";
    const auto* ompThreadLimitRequiredVal = "1";

    if (const auto* ompThreadLimitEnvVar = getenv(ompThreadLimit);
            ompThreadLimitEnvVar
            && strcmp(
                ompThreadLimitEnvVar,
                ompThreadLimitRequiredVal) == 0)
        return;

    if (setenv(
            ompThreadLimit, ompThreadLimitRequiredVal, true) == -1) {
        fprintf(
            stderr,
            "setenv(\"%s\", ...): %s\n",
            ompThreadLimit,
            dpso::os::getErrnoMsg(errno).c_str());
        exit(EXIT_FAILURE);
    }

    execvp(*argv, argv);

    fprintf(
        stderr,
        "execvp(\"%s\", ...): %s",
        *argv,
        dpso::os::getErrnoMsg(errno).c_str());
    exit(EXIT_FAILURE);
}
