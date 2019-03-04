
/**
 * \file
 * String formatting
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Argument for dpsoStrNamedFormat().
 */
struct DpsoFormatArg {
    const char* name;
    const char* str;
};


/**
 * Python-style named string formatting.
 *
 * The function formats string similar to Python's str.format(),
 * except it only supports named arguments. A partition in the str
 * enclosed in braces defines a name to be replaced with the
 * corresponding entry in the args array. For example, "name {value}".
 * To insert a brace as is, mention it twice.
 *
 * This function is mainly intended to be used for translatable
 * strings to provide reorderable and human-readable arguments instead
 * of printf-like magic numbers.
 */
const char* dpsoStrNamedFormat(
    const char* str,
    const struct DpsoFormatArg* args, int numArgs);


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus


#include <initializer_list>


// A helper for C++ that allows to pass arguments in place instead of
// constructing a separate array.
inline const char* dpsoStrNamedFormat(
    const char* str,
    std::initializer_list<const DpsoFormatArg> args)
{
    return dpsoStrNamedFormat(str, args.begin(), args.size());
}


#endif
