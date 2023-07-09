
#pragma once

#include <string>


namespace dpso {


// Convert path from UTF-8 to system-specific encoding.
//
// This function is intended to be used with third-party libraries
// that don't handle UTF-8 paths on Windows. It's only necessary for
// compatibility with Windows versions older than 10 build 1903, as
// they don't support enabling UTF-8 via application manifests.
//
// On systems other than Windows, the function returns the path as is.
//
// Throws std::runtime_error.
std::string convertPathFromUtf8ToSys(const char* utf8Path);


}
