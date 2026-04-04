// There's no universal way to find the location of the executable on
// Unix-like systems. Most modern systems provide their own ways for
// that, like procfs on Linux, KERN_PROC_PATHNAME on FreeBSD, etc.
//
// We use argv[0] in the general case. While it's not as robust as a
// platform-specific approach, it's definitely more portable and
// works in all normal invocation scenarios.

#include "exe_path.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"
#include "dpso_utils/unix/exe_path.h"

#include "toplevel_argv0.h"


namespace ui {


static std::string exePath;


bool initExePath(const char* argv0)
{
    const auto* toplevelArgv0 = getToplevelArgv0(argv0);

    try {
        exePath = dpso::unix::getExePath(toplevelArgv0);
        return true;
    } catch (dpso::os::Error& e) {
        dpso::setError(
            "unix::getExePath(\"{}\"): {}", toplevelArgv0, e.what());
        return false;
    }
}


const std::string& getExePath()
{
    return exePath;
}


}
