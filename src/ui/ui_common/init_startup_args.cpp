#include "init_startup_args.h"

#include <cstdlib>
#include <string_view>

#include "dpso_utils/error_get.h"
#include "dpso_utils/str_stdio.h"

#include "app_info.h"
#include "cmdline_cmd_autostart.h"
#include "cmdline_opts.h"
#include "toplevel_argv0.h"


using namespace dpso;


namespace ui {


static void printHelp(std::string_view argv0)
{
    str::print("{} {}\n\n", uiAppName, uiAppVersion);
    str::print("Usage\n");
    str::print("    {} [options...]\n", argv0);
    str::print("    {} command action\n\n", argv0);

    str::print(
        "Options\n"
        "\n"
        "  -help\n"
        "      Print this help and exit.\n"
        "  {}\n"
        "      Start the program with the hidden window. The window\n"
        "      will either be hidden to the notification area or\n"
        "      minimized if the notification area icon is disabled.\n"
        "  -version\n"
        "      Print version information and exit.\n"
        "\n",
        cmdLineOptHide);

    str::print(
        "Commands\n"
        "\n"
        "  autostart\n"
        "      Manage the application autostart. If autostart is\n"
        "      enabled, the program will start automatically with\n"
        "      the {} option when you log on to the system.\n"
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
            std::string_view name;
            bool (&fn)(
                std::string_view argv0, std::string_view action);
        } commands[]{
            {"autostart", cmdLineCmdAutostart}
        };

        const std::string_view cmdName{argv[1]};

        for (const auto& cmd : commands) {
            if (cmd.name != cmdName)
                continue;

            if (argc != 3) {
                str::print(
                    stderr,
                    "Command \"{}\" expects a single action.\n",
                    cmdName);
                std::exit(EXIT_FAILURE);
            }

            if (cmd.fn(argv[0], argv[2]))
                std::exit(EXIT_SUCCESS);

            str::print(stderr, "{}.\n", dpsoGetError());
            std::exit(EXIT_FAILURE);
        }

        str::print(
            stderr,
            "Unknown command \"{}\". Use \"-help\" for a list of "
            "available commands.\n",
            cmdName);
        std::exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "-help") {
            printHelp(getToplevelArgv0(argv[0]));
            std::exit(EXIT_SUCCESS);
        } else if (arg == "-version") {
            str::print("{} {}\n", uiAppName, uiAppVersion);
            std::exit(EXIT_SUCCESS);
        } else if (arg == cmdLineOptHide)
            result.hide = true;
        else {
            str::print(
                stderr,
                "Unknown option \"{}\". Use \"-help\" for a list of "
                "available options.\n",
                arg);
            std::exit(EXIT_FAILURE);
        }
    }

    return result;
}


}
