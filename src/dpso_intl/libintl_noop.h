
/**
 * \file No-op variants of functions from libintl.h to be used when
 * ENABLE_NLS is disabled.
 */

#pragma once


#if !ENABLE_NLS


#ifdef __cplusplus
extern "C" {
#endif


const char* gettext(const char* msgId);
const char* ngettext(
    const char* msgId1, const char* msgId2, unsigned long n);
const char* textdomain(const char* domainName);
const char* bind_textdomain_codeset(
    const char* domainName, const char* codeset);


#ifdef __cplusplus
}
#endif


#endif  // !ENABLE_NLS
