#include "init_extra.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"


namespace ui {


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
// variable and restart the program directly from the executable.
//
// [1] https://github.com/tesseract-ocr/tesstrain/issues/259


bool initStart(int argc, char* argv[])
{
    (void)argc;

    const auto* ompThreadLimit = "OMP_THREAD_LIMIT";
    const auto* ompThreadLimitRequiredVal = "1";

    if (const auto* ompThreadLimitEnvVar = getenv(ompThreadLimit);
            ompThreadLimitEnvVar
            && strcmp(
                ompThreadLimitEnvVar,
                ompThreadLimitRequiredVal) == 0)
        return true;

    if (setenv(
            ompThreadLimit, ompThreadLimitRequiredVal, true) == -1) {
        dpso::setError(
            "setenv(\"{}\", ...): {}",
            ompThreadLimit, dpso::os::getErrnoMsg(errno));
        return false;
    }

    execvp(*argv, argv);

    dpso::setError(
        "execvp(\"{}\", ...): {}",
        *argv, dpso::os::getErrnoMsg(errno));
    return false;
}


bool initEnd(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    return true;
}


}
