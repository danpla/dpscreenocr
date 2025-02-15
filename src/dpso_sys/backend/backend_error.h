#pragma once

#include <stdexcept>


namespace dpso::backend {


class BackendError : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
