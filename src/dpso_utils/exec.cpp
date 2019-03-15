
#include "exec.h"


#if defined(__unix__)

#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


void dpsoExec(
    const char* exePath, const char* arg, int waitToComplete)
{
    const auto pid = fork();

    switch (pid) {
        case -1:
            return;
            break;
        case 0: {
            const char* args[] = {exePath, arg, nullptr};
            execvp(args[0], (char* const*)args);
            // Use _Exit() instead of exit() since we don't want to
            // call C++ destructors. In particular, this removes
            // messages from Tesseract 4 ("~ObjectCache(): WARNING!
            // LEAK!") and std::thread ("terminate called without an
            // active exception").
            std::_Exit(EXIT_FAILURE);
            break;
        }
        default:
            if (waitToComplete)
                waitpid(pid, nullptr, 0);
            break;
    }
}


#else


void dpsoExec(
    const char* exePath, const char* arg, int waitToComplete)
{
    (void)exePath;
    (void)arg;
    (void)waitToComplete;
}


#endif
