
#pragma once


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


#define REGISTER_TEST(NAME, FN) \
static test::Runner FN ## TestRunner(NAME, FN)


void failure();


int getNumFailures();


}
