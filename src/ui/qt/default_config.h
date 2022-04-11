
// Default values for macros.

#pragma once

#include <QtGlobal>


#if defined(Q_OS_UNIX) \
        && !defined(Q_OS_DARWIN) \
        && QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) \
        && QT_VERSION < QT_VERSION_CHECK(5, 3, 2)
    #ifndef DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
        #define DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND 1
    #endif
#else
    #undef DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
#endif


#ifndef DPSO_QT_LOCAL_DATA
    #define DPSO_QT_LOCAL_DATA 1
#endif
