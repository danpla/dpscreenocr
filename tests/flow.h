
#pragma once


namespace test {


class Runner {
public:
    static const Runner* getFirst();
    const Runner* getNext() const;

    Runner(const char* name, void (&fn)());

    const char* getName() const;
    void run() const;
private:
    static Runner* list;

    const char* name;
    void (*fn)();
    Runner* next;
};


#define REGISTER_TEST(NAME, FN) \
static test::Runner FN ## TestRunner(NAME, FN)


// Failures are not fatal by default.
void setFailureIsFatal(bool newIsFatal);


void failure();


int getNumFailures();


}
