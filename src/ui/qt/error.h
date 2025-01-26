#pragma once

#include <stdexcept>


namespace ui::qt {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


}
