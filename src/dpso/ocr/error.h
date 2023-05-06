
#pragma once

#include <stdexcept>


namespace dpso::ocr {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
