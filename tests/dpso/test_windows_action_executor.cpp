#include <stdexcept>

#include "dpso/backend/windows/execution_layer/action_executor.h"

#include "flow.h"


namespace {


void testExceptions()
{
    using namespace dpso::backend;

    class TestException : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    ActionExecutor actionExecutor;

    try {
        execute(actionExecutor, []{ throw TestException{""}; });
        test::failure(
            "testExceptions: Exception was not propagated by the "
            "executor");
        return;
    } catch (TestException&) {
    }

    // There was a bug where an exception stored in std::exception_ptr
    // under the hood was not cleared before being rethrown, and thus
    // thrown for all further successful execute() calls.
    try {
        execute(actionExecutor, []{});
    } catch (TestException&) {
        test::failure(
            "testExceptions: The previously propagated exception was "
            "rethrown on a successful call");
    }
}


void testWindowsActionExecutor()
{
    testExceptions();
}


}


REGISTER_TEST(testWindowsActionExecutor);
