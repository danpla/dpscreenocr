
#pragma once

/*
 * At this moment, all paths are absolute an are intended mainly for
 * Unix-like systems. Once we implement relocatibility on on these
 * systems, the paths become relative and we can use these constants
 * on all platforms. The values will obviously still be
 * platform-specific, e.g. on Windows we want to put icons in "icons"
 * rather than in "share/icons".
 */


/*
 * Path to localization data to be passed to bindtextdomain(). Mainly
 * intended for non-Apple Unix-like systems, where it's an absolute
 * path pointing to installation prefix + "share/locale". May be empty
 * on platforms that don't use it.
 */
extern const char* const uiLocaleDir;


/*
 * Path to directory with documents. Mainly intended for non-Apple
 * Unix-like systems, where it's an absolute path pointing to
 * installation prefix + "share/doc". May be empty on platforms that
 * don't use it.
 */
extern const char* const uiDocDir;
