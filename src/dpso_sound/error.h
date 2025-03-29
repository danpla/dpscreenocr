#pragma once

#include <stdexcept>


namespace dpso::sound {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
