#include "autostart_default.h"

#include <iterator>

#include "app_info.h"
#include "cmdline_opts.h"
#include "exe_path.h"
#include "file_names.h"


UiAutostart* uiAutostartCreateDefault(void)
{
    const char* args[]{
        ui::getToplevelExePath().c_str(), ui::cmdLineOptHide};

    const UiAutostartArgs autostartArgs{
        uiAppName, uiAppFileName, args, std::size(args)};

    return uiAutostartCreate(&autostartArgs);
}
