#include "exe_path.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/unix/exe_path.h"

#include "toplevel_argv0.h"


namespace ui {
namespace {


std::string toplevelExePath;
std::string exePath;


bool initExePath(std::string& path, const char* argv0)
{
    try {
        path = dpso::unix::getExePath(argv0);
        return true;
    } catch (dpso::os::Error& e) {
        dpso::setError(
            "unix::getExePath(\"{}\"): {}", argv0, e.what());
        return false;
    }
}


}


bool initExePath(const char* argv0)
{
    return
        initExePath(toplevelExePath, getToplevelArgv0(argv0))
        && initExePath(exePath, argv0);
}


const std::string& getToplevelExePath()
{
    return toplevelExePath;
}


const std::string& getExePath()
{
    return exePath;
}


}
