#include "os_c.h"

#include <chrono>
#include <thread>
#include <vector>

#include "error_set.h"
#include "os.h"


char dpsoGetDirSeparator(void)
{
    return dpso::os::dirSeparators[0];
}


void dpsoSleep(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds{milliseconds});
}


bool dpsoExec(
    const char* exePath, const char* const args[], size_t numArgs)
{
    const std::vector<std::string_view> argsSv{args, args + numArgs};

    try {
        dpso::os::exec(exePath, argsSv.data(), argsSv.size());
        return true;
    } catch (dpso::os::Error& e) {
        dpso::setError("{}", e.what());
        return false;
    }
}
