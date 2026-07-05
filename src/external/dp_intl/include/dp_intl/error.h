#pragma once

#include <stdexcept>


namespace dp::intl {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


class FileNotFoundError : public Error {
    using Error::Error;
};


}
