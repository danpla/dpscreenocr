
#include "exec.h"

#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


static void exec(const char* exePath, const char* arg)
{
    const char* args[] = {exePath, arg, nullptr};
    execvp(args[0], (char* const*)args);
}


void dpsoExec(
    const char* exePath, const char* arg, int waitToComplete)
{
    const auto childPid = fork();
    if (childPid == -1)
        return;

    if (childPid != 0) {
        waitpid(childPid, nullptr, 0);
        return;
    }

    if (waitToComplete) {
        exec(exePath, arg);
        std::_Exit(EXIT_FAILURE);
    }

    const auto grandchildPid = fork();
    if (grandchildPid == 0) {
        exec(exePath, arg);
        std::_Exit(EXIT_FAILURE);
    }

    // No waitpid(grandchildPid, ...) here: grandchildPid will be
    // adopted by init.
    std::_Exit(grandchildPid == -1 ? EXIT_FAILURE : EXIT_SUCCESS);
}
