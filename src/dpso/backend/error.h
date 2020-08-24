
#pragma once

#include <stdexcept>


namespace dpso {
namespace backend {


class BackendError : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
}
