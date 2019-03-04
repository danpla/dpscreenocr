
#include "flow.h"

#include <cstdio>
#include <cstdlib>


namespace test {


static bool failureIsFatal;
static int numFailures;


void setFailureIsFatal(bool newIsFatal)
{
    failureIsFatal = newIsFatal;
}


void failure()
{
    if (failureIsFatal)
        std::exit(EXIT_FAILURE);
    else
        ++numFailures;
}


int getNumFailures()
{
    return numFailures;
}


}
