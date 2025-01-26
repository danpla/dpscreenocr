#include "autostart_default_args.h"

#include <iterator>

#include "app_info.h"
#include "cmdline_opts.h"
#include "exe_path.h"
#include "file_names.h"


void uiAutostartGetDefaultArgs(UiAutostartArgs* autostartArgs)
{
    if (!autostartArgs)
        return;

    static const char* args[]{
        ui::getToplevelExePath().c_str(), ui::cmdLineOptHide};

    *autostartArgs = {
        uiAppName, uiAppFileName, args, std::size(args)};
}
