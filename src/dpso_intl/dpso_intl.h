
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
