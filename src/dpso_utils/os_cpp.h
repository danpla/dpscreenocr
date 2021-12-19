
/**
 * \file
 * C++ helpers for os.h
 */

#pragma once

#include <memory>

#include "os.h"


namespace dpso {


/**
 * Deleter for a FILE* smart pointer.
 */
struct StdFileCloser {
    void operator()(FILE* fp) const
    {
        if (fp)
            fclose(fp);
    }
};


using StdFileUPtr = std::unique_ptr<FILE, StdFileCloser>;


/**
 * dpsoFopenUtf8() wrapper.
 */
inline StdFileUPtr fopenUtf8(const char* fileName, const char* mode)
{
    return StdFileUPtr{dpsoFopenUtf8(fileName, mode)};
}


}
