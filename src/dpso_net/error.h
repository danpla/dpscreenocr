#pragma once

#include <stdexcept>


namespace dpso::net {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


// The connection either could not be established or was terminated.
// This error is intended as a hint to show the end user a more
// friendly error message, such as "check your network connection and
// try again".
class ConnectionError : public Error {
    using Error::Error;
};


}
