
#include "init_startup_args.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dpso_utils/error_get.h"

#include "app_info.h"
#include "cmdline_cmd_autostart.h"
#include "cmdline_opts.h"
#include "toplevel_argv0.h"


namespace ui {


static void printHelp(const char* argv0)
{
    std::printf("%s %s\n\n", uiAppName, uiAppVersion);
    std::fputs("Usage\n", stdout);
    std::printf("    %s [options...]\n", argv0);
    std::printf("    %s command action\n\n", argv0);

    std::printf(
        "Options\n"
        "\n"
        "  -help\n"
        "      Print this help and exit.\n"
        "  %s\n"
        "      Start the program with the hidden window. The window\n"
        "      will either be hidden to the notification area or\n"
        "      minimized if the notification area icon is disabled.\n"
        "  -version\n"
        "      Print version information and exit.\n"
        "\n",
        cmdLineOptHide);

    std::printf(
        "Commands\n"
        "\n"
        "  autostart\n"
        "      Manage the application autostart. If autostart is\n"
        "      enabled, the program will start automatically with\n"
        "      the %s option when you log on to the system.\n"
        "\n"
        "      Actions\n"
        "        on\n"
        "            Enable autostart.\n"
        "        off\n"
        "            Disable autostart.\n"
        "        query\n"
        "            Print \"on\" or \"off\" depending on whether\n"
        "            autostart is enabled.\n",
        cmdLineOptHide);
}


UiStartupArgs initStartupArgs(int argc, char* argv[])
{
    UiStartupArgs result{};

    // Comand mode.
    if (argc > 1 && *argv[1] != '-') {
        const struct {
            const char* name;
            bool (&fn)(const char* argv0, const char* action);
        } commands[]{
            {"autostart", cmdLineCmdAutostart}
        };

        const auto* cmdName = argv[1];

        for (const auto& cmd : commands) {
            if (std::strcmp(cmd.name, cmdName) != 0)
                continue;

            if (argc != 3) {
                std::fprintf(
                    stderr,
                    "Command \"%s\" expects a single action.\n",
                    cmdName);
                std::exit(EXIT_FAILURE);
            }

            if (cmd.fn(argv[0], argv[2]))
                std::exit(EXIT_SUCCESS);

            std::fprintf(stderr, "%s.\n", dpsoGetError());
            std::exit(EXIT_FAILURE);
        }

        std::fprintf(
            stderr,
            "Unknown command \"%s\". Use \"-help\" for a list of "
            "available commands.\n",
            cmdName);
        std::exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        const auto* arg = argv[i];

        if (std::strcmp(arg, "-help") == 0) {
            printHelp(getToplevelArgv0(argv[0]));
            std::exit(EXIT_SUCCESS);
        } else if (std::strcmp(arg, "-version") == 0) {
            std::printf("%s %s\n", uiAppName, uiAppVersion);
            std::exit(EXIT_SUCCESS);
        } else if (std::strcmp(arg, cmdLineOptHide) == 0)
            result.hide = true;
        else {
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
