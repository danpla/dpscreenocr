
#pragma once

// Default values for macros.


#define NEED_SET_WINDOW_TITLE_WORKAROUND() \
    (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) \
        && QT_VERSION < QT_VERSION_CHECK(5, 3, 2))

#ifndef DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    #if NEED_SET_WINDOW_TITLE_WORKAROUND()
        #define DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND 1
    #else
        #define DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND 0
    #endif
#else
    #if !NEED_SET_WINDOW_TITLE_WORKAROUND()
        #undef DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    #endif
#endif

#undef NEED_SET_WINDOW_TITLE_WORKAROUND


#ifndef DPSO_QT_RCC_ICONS
    #define DPSO_QT_RCC_ICONS 1
#endif


#ifndef DPSO_QT_LOCAL_DATA
    #define DPSO_QT_LOCAL_DATA 1
#endif
