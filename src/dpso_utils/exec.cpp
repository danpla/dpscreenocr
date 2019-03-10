
#include "exec.h"


#if defined(__unix__)

#include <cstdlib>
#include <unistd.h>


void dpsoExec(const char* exePath, const char* arg1)
{
    if (fork() != 0)
        return;

    const char* args[] = {exePath, arg1, nullptr};
    execvp(args[0], (char* const*)args);

    // Use _Exit() instead of exit() since we don't want to call C++
    // destructors. In particular, this removes messages from
    // Tesseract 4 ("~ObjectCache(): WARNING! LEAK!") and std::thread
    // ("terminate called without an active exception").
    std::_Exit(EXIT_FAILURE);
}


#else


void dpsoExec(const char* exePath, const char* arg1)
{
    (void)exePath;
    (void)arg1;
}


#endif
