#include "flow.h"

#include <cstdlib>

#include "dpso_utils/str_stdio.h"


using namespace dpso;


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


Runner::Runner(std::string_view name, void (&fn)())
    : name{name}
    , fn{fn}
    , next{}
{
    // Link alphabetically.
    auto** pos = &list;
    while (*pos && (*pos)->name < name)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;

    ++numRunners;
}


std::string_view Runner::getName() const
{
    return name;
}


void Runner::run() const
{
    fn();
}


static int numFailures;


void failure(
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    ++numFailures;
    str::print(stderr, fmt, args);
    str::print(stderr, "\n");
}


int getNumFailures()
{
    return numFailures;
}


void fatalError(
    std::string_view fmt,
    std::initializer_list<std::string_view> args)
{
    str::print(stderr, "FATAL ERROR\n");
    str::print(stderr, fmt, args);
    str::print(stderr, "\n");
    std::exit(EXIT_FAILURE);
}


}
