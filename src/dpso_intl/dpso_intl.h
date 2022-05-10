
/**
 * \file Helpers for libintl.h
 *
 * This file mimics gettext.h, but contains only routines we actually
 * need. It should only be included if you don't use gettext.h.
 */

#pragma once


#if ENABLE_NLS

#include <libintl.h>

#else

#define gettext(msgId) msgId
#define ngettext(msgId1, msgId2, N) N == 1 ? msgId1 : msgId2
#define textdomain(domainName) ((const char*)domainName)
#define bindtextdomain(domainName, dirName) ((const char*)dirName)
#define bind_textdomain_codeset(domainName, codeset) \
    ((const char*)codeset)

#endif


#ifdef __cplusplus
extern "C" {
#endif


#define pgettext(msgCtx, msgId) \
    pgettext_aux(msgCtx "\004" msgId, msgId)


const char* pgettext_aux(const char* msgCtxId, const char* msgId);


const char* pgettext_expr(const char* msgCtx, const char* msgId);


#define npgettext(msgCtx, msgId1, msgId2, N) \
    npgettext_aux(msgCtx "\004" msgId1, msgId1, msgId2, N)


const char* npgettext_aux(
    const char* msgCtxId, const char* msgId1, const char* msgId2,
    unsigned long n);


const char* npgettext_expr(
    const char* msgCtx, const char* msgId1, const char* msgId2,
    unsigned long n);


#ifdef __cplusplus
}
#endif
