
#pragma once

#include <fmt/core.h>


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
void vFailure(fmt::string_view format, fmt::format_args args);

template<typename... T>
void failure(fmt::format_string<T...> format, T&&... args)
{
    vFailure(format, fmt::make_format_args(args...));
}


// Return the number of failure() calls.
int getNumFailures();


// Report an abnormal error and exit. The function is intended for
// errors that are not directly related to test cases, e.g. an IO
// error when setting up a test.
[[noreturn]]
void vFatalError(fmt::string_view format, fmt::format_args args);

template<typename... T>
[[noreturn]]
void fatalError(fmt::format_string<T...> format, T&&... args)
{
    vFatalError(format, fmt::make_format_args(args...));
}


}
