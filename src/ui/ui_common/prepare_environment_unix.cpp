
#include "prepare_environment.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void uiPrepareEnvironment(char* argv[])
{
    const auto* ompThreadLimit = "OMP_THREAD_LIMIT";
    const auto* ompThreadLimitRequiredVal = "1";

    const auto* ompThreadLimitEnvVar = getenv(ompThreadLimit);
    if (ompThreadLimitEnvVar
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
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    execvp(*argv, argv);

    fprintf(
        stderr, "execvp(\"%s\", ...): %s", *argv, strerror(errno));
    exit(EXIT_FAILURE);
}
