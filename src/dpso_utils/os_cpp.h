
/**
 * \file
 * C++ helpers for os.h
 */

#pragma once


#ifdef __cplusplus


#include <cstdio>
#include <memory>


namespace dpso {


/**
 * Deleter for a FILE* smart pointer.
 */
struct StdFileCloser {
    void operator()(std::FILE* fp) const
    {
        if (fp)
            std::fclose(fp);
    }
};


using StdFileUPtr = std::unique_ptr<std::FILE, StdFileCloser>;


}


#endif
