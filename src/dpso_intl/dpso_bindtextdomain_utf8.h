
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * bindtextdomain() that accepts dirName in UTF-8.
 *
 * This function is a wrapper that calls wbindtextdomain() on Windows
 * (requires libintl >= 0.21.0) and normal bindtextdomain() on other
 * platforms. It's mainly intended to support Windows older than 10;
 * since Windows 10 version 1903, the application manifest
 * (<activeCodePage>) is a more robust and non-intrusive way to enable
 * Unicode support in all third-party libraries.
 *
 * The (w)bindtextdomain() documentation says that the returned string
 * is only invalidated if the function is called with the same domain
 * name. Emulating this behavior in our UTF-8 wrapper on Windows would
 * require maintaining a mapping of results from UTF-16 to UTF-8. This
 * complication is probably not worth the trouble, so the function
 * returns nothing.
 */
void bindtextdomainUtf8(const char* domainName, const char* dirName);


#ifdef __cplusplus
}
#endif
