#pragma once


// DP_INTL_EXPORT is for all symbols that need to be exported from a
// shared library.
//
// DP_INTL_VISIBLE_SYMBOL is for symbols that are not otherwise
// exported (e.g., those that are header-only), but are used by RTTI.
// This is mainly needed for exception classes on Unix-like platforms,
// where catching an exception thrown by another binary can require a
// typeinfo lookup (this is no longer true for modern GCC/Glibc [1],
// but may still be relevant for older systems and other platforms).
// For details, see the section on the exception handling at
// https://gcc.gnu.org/wiki/Visibility
//
// Although on Unix-like platforms DP_INTL_VISIBLE_SYMBOL is exactly
// the same as DP_INTL_EXPORT, we can't replace the former with the
// latter for all platforms. On Windows (where marking a header-only
// class with dllexport is unnecessary and harmless) this will give a
// MSVC warning C4275 when the class derives from another class that
// is not marked as dllexport. In our case, this will be a class
// derived from a standard library exception. The C4275 documentation
// explicitly states that this warning can be ignored when deriving
// from a standard library type [2], but we don't want to mess with
// the warning suppression pragmas; instead, we have a separate
// DP_INTL_VISIBLE_SYMBOL macro that expands to nothing on Windows.
//
// [1]: The Glibc manual explicitly states that exception handling and
//   RTTI in the GNU toolchain is not address-significant, and
//   therefore is not affected by the symbol binding behavior of the
//   dynamic linker. Types with the same name are always considered
//   the same from an exception-handling or RTTI perspective, even if
//   the symbol is hidden:
//
//   https://sourceware.org/glibc/manual/latest/html_node/Dynamic-Linker-Hardening.html#Restricted-Dynamic-Linker-Features
//
// [2]: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275


#ifdef DP_INTL_SHARED_LIB
    #ifdef _WIN32
        #ifdef DP_INTL_EXPORTS
            #define DP_INTL_EXPORT __declspec(dllexport)
        #else
            #define DP_INTL_EXPORT __declspec(dllimport)
        #endif
    #elif defined(__GNUC__)
        #define DP_INTL_EXPORT __attribute__((visibility("default")))
        #define DP_INTL_VISIBLE_SYMBOL DP_INTL_EXPORT
    #endif
#endif


#ifndef DP_INTL_EXPORT
    #define DP_INTL_EXPORT
#endif


#ifndef DP_INTL_VISIBLE_SYMBOL
    #define DP_INTL_VISIBLE_SYMBOL
#endif
