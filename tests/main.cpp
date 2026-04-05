#include <cstdlib>
#include <cstdio>

#include "dpso_utils/str.h"
#include "dpso_utils/str_stdio.h"

#include "flow.h"


using namespace dpso;


int main()
{
    int curRunnerNum{};

    for (const auto* runner = test::Runner::getFirst();
            runner;
            runner = runner->getNext()) {
        str::print(
            "{}/{}: {}\n",
            str::justifyRight(str::toStr(++curRunnerNum), 2),
            test::Runner::getNumRunners(),
            runner->getName());
        // Flush to make sure that failure() messages (written to
        // stderr) are nested under the test name.
        std::fflush(stdout);
        runner->run();
    }

    str::print("===\n");
    const auto numFailures = test::getNumFailures();
    if (numFailures == 0)
        str::print("Everything is OK\n");
    else
        str::print(
            "{} failure{}\n",
            numFailures,
            numFailures > 1 ? "s" : "");

    return numFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
