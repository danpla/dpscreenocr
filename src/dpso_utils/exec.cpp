
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
    std::exit(EXIT_FAILURE);
}


#else


void dpsoExec(const char* exePath, const char* arg1)
{
    (void)exePath;
    (void)arg1;
}


#endif
