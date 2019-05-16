
#include "exec.h"

#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


void dpsoExec(
    const char* exePath, const char* arg, int waitToComplete)
{
    const auto pid = fork();
    if (pid == -1)
        return;

    if (pid == 0) {
        const char* args[] = {exePath, arg, nullptr};
        execvp(args[0], (char* const*)args);
        // Use _Exit() instead of exit() since we don't want to call
        // C++ destructors. In particular, this removes messages from
        // Tesseract (~ObjectCache(): WARNING! LEAK!) and std::thread
        // (terminate called without an active exception).
        std::_Exit(EXIT_FAILURE);
    }

    if (waitToComplete)
        waitpid(pid, nullptr, 0);
}
