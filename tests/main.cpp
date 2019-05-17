
#include <cstdio>
#include <cstdlib>

#include "flow.h"


int main()
{
    for (const auto* runner = test::Runner::getFirst();
            runner;
            runner = runner->getNext())
        runner->run();

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
