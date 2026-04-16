#include "exe_path.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/os_error.h"
#include "dpso_utils/windows/exe_path.h"


namespace ui {


static std::string exePath;


bool initExePath(std::string_view /*argv0*/)
{
    try {
        exePath = dpso::windows::getExePath();
        return true;
    } catch (dpso::os::Error& e) {
        dpso::setError("windows::getExePath(): {}", e.what());
        return false;
    }
}


const std::string& getExePath()
{
    return exePath;
}


}
