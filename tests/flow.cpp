
#include "flow.h"

#include <cstdarg>
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


int Runner::getNumRunners()
{
    return numRunners;
}


Runner* Runner::list;
int Runner::numRunners;


Runner::Runner(const char* name, void (&fn)())
    : name{name}
    , fn{fn}
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


void failure(const char* fmt, ...)
{
    ++numFailures;

    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
}


int getNumFailures()
{
    return numFailures;
}


void fatalError(const char* fmt, ...)
{
    std::fputs("FATAL ERROR\n", stderr);

    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);

    std::exit(EXIT_FAILURE);
}


}
