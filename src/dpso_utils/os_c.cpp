
#include "os_c.h"

#include <chrono>
#include <thread>

#include "error.h"
#include "os.h"


const char* const dpsoDirSeparators = dpso::os::dirSeparators;


void dpsoSleep(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds{milliseconds});
}


bool dpsoExec(
    const char* exePath, const char* const args[], size_t numArgs)
{
    try {
        dpso::os::exec(exePath, args, numArgs);
        return true;
    } catch (dpso::os::Error& e) {
        dpsoSetError("%s", e.what());
        return false;
    }
}
