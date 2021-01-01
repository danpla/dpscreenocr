
#include "flow.h"

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


int Runner::getNumRunners()
{
    return numRunners;
}


Runner* Runner::list;
int Runner::numRunners;


Runner::Runner(const char* name, void (&fn)())
    : name{name}
    , fn{&fn}
    , next{}
{
    // Link alphabetically.
    auto** pos = &list;
    while (*pos && std::strcmp((*pos)->name, name) < 0)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;

    ++numRunners;
}


const char* Runner::getName() const
{
    return name;
}


void Runner::run() const
{
    fn();
}


static int numFailures;


void failure()
{
    ++numFailures;
}


int getNumFailures()
{
    return numFailures;
}


}
