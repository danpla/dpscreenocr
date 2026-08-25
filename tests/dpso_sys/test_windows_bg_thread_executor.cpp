#include <stdexcept>

#include <thread>

#include "dpso_sys/backend/windows/bg_thread_executor.h"

#include "flow.h"


namespace {


using namespace dpso::backend::windows;


void testExecution()
{
    BgThreadExecutor executor;

    const auto mainThreadId = std::this_thread::get_id();

    executor(
        [&]
        {
            const auto call1ThreadId = std::this_thread::get_id();
            if (call1ThreadId == mainThreadId) {
                test::failure(
                    "BgThreadExecutor runs callables in the main "
                    "thread");
                return;
            }

            executor(
                [&]
                {
                    const auto call2ThreadId =
                        std::this_thread::get_id();
                    if (call2ThreadId != call1ThreadId) {
                        test::failure(
                            "Level-2 call does not run in the same "
                            "thread as the parent");
                        return;
                    }

                    executor(
                    [&]
                    {
                        const auto call3ThreadId =
                            std::this_thread::get_id();
                        if (call3ThreadId != call2ThreadId) {
                            test::failure(
                                "Level-3 call does not run in the "
                                "same thread as the parent");
                            return;
                        }
                    });
                });
        });
}


void testExceptions()
{
    class TestException : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    BgThreadExecutor executor;

    try {
        executor([]{ throw TestException{""}; });
        test::failure(
            "testExceptions: Exception was not propagated by the "
            "executor");
        return;
    } catch (TestException&) {
    }

    // There was a bug where an exception stored in std::exception_ptr
    // under the hood of BgThreadExecutor was not cleared before being
    // rethrown, and thus thrown for all further successful operator()
    // calls.
    try {
        executor([]{});
    } catch (TestException&) {
        test::failure(
            "testExceptions: The previously propagated exception was "
            "rethrown on a successful call");
    }
}


void testWindowsBgThreadExecutor()
{
    testExecution();
    testExceptions();
}


}


REGISTER_TEST(testWindowsBgThreadExecutor);
