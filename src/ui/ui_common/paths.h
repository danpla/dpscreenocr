
#pragma once


/*
 * Path to localization data to be passed to bindtextdomain(). Mainly
 * intended for non-Apple Unix-like systems, where it's an absolute
 * path pointing to installation prefix + "share/locale". On platforms
 * that don't use it, it may be empty.
 */
extern const char* const localeDir;
