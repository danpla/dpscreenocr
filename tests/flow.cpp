
#include "flow.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace test {


const Runner* Runner::getFirst()
{
    return list;
}


const Runner* Runner::getNext() const
{
    return next;
}


Runner* Runner::list;


Runner::Runner(const char* name, void (&fn)())
    : name {name}
    , fn {&fn}
    , next {}
{
    // Link alphabetically.
    auto** pos = &list;
    while (*pos && std::strcmp((*pos)->name, name) < 0)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;
}


const char* Runner::getName() const
{
    return name;
}


void Runner::run() const
{
    fn();
}


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
