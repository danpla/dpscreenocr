#pragma once

#include <memory>
#include <string_view>

#include "dp_intl/count_t.h"


namespace dp::intl::plural {


// Compiler and evaluator for Gettext plural form expressions.
class Eval {
public:
    // Compile a plural form expression.
    //
    // Throws Error.
    explicit Eval(std::string_view expr);
    ~Eval();

    Eval(const Eval&) = delete;
    Eval& operator=(const Eval&) = delete;

    Eval(Eval&&) = delete;
    Eval& operator=(Eval&&) = delete;

    // Throws Error on logic errors, such as division by 0 and integer
    // overflow.
    CountT operator()(CountT n) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


}
