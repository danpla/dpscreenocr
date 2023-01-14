
#include "exec.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


void dpsoExec(const char* exePath, const char* arg)
{
    const auto pid = fork();
    if (pid == -1)
        return;

    if (pid == 0) {
        const char* args[] = {exePath, arg, nullptr};
        execvp(args[0], (char* const*)args);
        _Exit(EXIT_FAILURE);
    }

    waitpid(pid, nullptr, 0);
}
