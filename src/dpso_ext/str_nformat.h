#pragma once

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Argument for dpsoStrNFormat().
 */
typedef struct DpsoStrNFormatArg {
    const char* name;
    const char* str;
} DpsoStrNFormatArg;


/**
 * Python-style named string formatting.
 *
 * The function formats string similar to Python's str.format(),
 * except it only supports named arguments. A partition in the str
 * enclosed in braces defines a name to be replaced with the
 * corresponding entry in the args array. To insert a brace as is,
 * mention it twice.
 *
 * This function is mainly intended to be used for translatable
 * strings to provide reorderable and human-readable arguments.
 */
const char* dpsoStrNFormat(
    const char* str, const DpsoStrNFormatArg* args, size_t numArgs);


#ifdef __cplusplus
}


#include <initializer_list>


inline const char* dpsoStrNFormat(
    const char* str,
    std::initializer_list<DpsoStrNFormatArg> args)
{
    return dpsoStrNFormat(str, args.begin(), args.size());
}


#endif
