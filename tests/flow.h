
#pragma once

#include "dpso_utils/str.h"


namespace test {


class Runner {
public:
    static const Runner* getFirst();
    const Runner* getNext() const;

    static int getNumRunners();

    Runner(const char* name, void (&fn)());

    const char* getName() const;
    void run() const;
private:
    static Runner* list;
    static int numRunners;

    const char* name;
    void (*fn)();
    Runner* next;
};


#define REGISTER_TEST(FN) \
static test::Runner FN ## TestRunner(#FN, FN)


// Report a test case failure, increment the failure counter, and
// continue.
void failure(
    const char* fmt, std::initializer_list<const char*> args);

template<typename... Args>
void failure(const char* fmt, const Args&... args)
{
    failure(fmt, {dpso::str::formatArg::get(args)...});
}


// Return the number of failure() calls.
int getNumFailures();


// Report an abnormal error and exit. The function is intended for
// errors that are not directly related to test cases, e.g. an IO
// error when setting up a test.
[[noreturn]]
void fatalError(
    const char* fmt, std::initializer_list<const char*> args);

template<typename... Args>
[[noreturn]]
void fatalError(const char* fmt, const Args&... args)
{
    fatalError(fmt, {dpso::str::formatArg::get(args)...});
}


}
