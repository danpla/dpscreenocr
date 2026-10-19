#pragma once

#include <stdexcept>

#include "dp_intl/impl/export.h"


namespace dp::intl {


class DP_INTL_VISIBLE_SYMBOL Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


class DP_INTL_VISIBLE_SYMBOL FileNotFoundError : public Error {
    using Error::Error;
};


}
