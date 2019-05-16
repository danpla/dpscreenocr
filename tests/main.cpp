
#include <cstdio>
#include <cstdlib>

#include "flow.h"
#include "test_cfg.h"
#include "test_geometry.h"
#include "test_history.h"
#include "test_hotkeys.h"
#include "test_str.h"
#include "test_str_format.h"

// TODO: Make a static plugin system so we don't have to include and
// call the tests explicitly.
#ifdef _WIN32
#include "test_windows_utils.h"
#endif


int main()
{
    testCfg();
    testGeometry();
    testHistory();
    testHotkeys();
    testStr();
    testStrFormat();
    #ifdef _WIN32
    testWindowsUtils();
    #endif

    const auto numFailures = test::getNumFailures();
    if (numFailures == 0)
        std::printf("Everything is OK\n");
    else
        std::printf(
            "===\n%i failure%s\n",
            numFailures,
            numFailures > 1 ? "s" : "");

    return numFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
