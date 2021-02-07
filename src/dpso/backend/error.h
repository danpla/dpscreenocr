
#pragma once

#include <stdexcept>


namespace dpso {
namespace backend {


/**
 * Backend error.
 *
 * Only expected to be thrown from the Backend's constructor.
 */
class BackendError : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
}
