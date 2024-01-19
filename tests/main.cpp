
#include <cstdlib>

#include <fmt/core.h>

#include "flow.h"


int main()
{
    int curRunnerNum{};

    for (const auto* runner = test::Runner::getFirst();
            runner;
            runner = runner->getNext()) {
        fmt::print(
            "{:3}/{}: {}\n",
            ++curRunnerNum, test::Runner::getNumRunners(),
            runner->getName());
        // Flush to make sure that failure() messages (written to
        // stderr) are nested under the test name.
        std::fflush(stdout);
        runner->run();
    }

    fmt::print("===\n");
    const auto numFailures = test::getNumFailures();
    if (numFailures == 0)
        fmt::print("Everything is OK\n");
    else
        fmt::print(
            "{} failure{}\n",
            numFailures,
            numFailures > 1 ? "s" : "");

    return numFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
