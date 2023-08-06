
#include "os_c.h"

#include <chrono>
#include <thread>

#include "os.h"


const char* const dpsoDirSeparators = dpso::os::dirSeparators;


void dpsoSleep(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds{milliseconds});
}

