
#include "init_startup_args.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "app_info.h"
#include "cmdline_opts.h"
#include "toplevel_argv0.h"


namespace ui {


static void printHelp(const char* argv0)
{
    std::printf("%s %s\n\n", uiAppName, uiAppVersion);
    std::printf("Usage: %s [options...]\n\n", argv0);

    std::printf(
        "  -help\n"
        "      Print this help and exit.\n"
        "  %s\n"
        "      Start the program with the hidden window. The window\n"
        "      will either be hidden to the notification area or\n"
        "      minimized if the notification area icon is disabled.\n"
        "  -version\n"
        "      Print version information and exit.\n",
        cmdLineOptHide);
}


UiStartupArgs initStartupArgs(int argc, char* argv[])
{
    UiStartupArgs result{};

    for (int i = 1; i < argc; ++i) {
        const auto* arg = argv[i];

        if (std::strcmp(arg, "-help") == 0) {
            printHelp(getToplevelArgv0(argv[0]));
            std::exit(EXIT_SUCCESS);
        } else if (std::strcmp(arg, "-version") == 0) {
            std::printf("%s %s\n", uiAppName, uiAppVersion);
            std::exit(EXIT_SUCCESS);
        } else if (std::strcmp(arg, cmdLineOptHide) == 0) {
            result.hide = true;
        } else {
            std::fprintf(
                stderr,
                "Unknown option \"%s\". Use \"-help\" for a list of "
                "available options.\n",
                arg);
            std::exit(EXIT_FAILURE);
        }
    }

    return result;
}


}
