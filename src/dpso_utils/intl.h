
/**
 * \file Helpers for libintl.h
 *
 * This file mostly mimics to gettext.h, but contains only routines
 * we actually use.
 *
 * The names of the functions and macros also match gettext.h. Still,
 * be aware that although variants with _expr postfix also come from
 * gettext.h, they are not in the list of standard keywords xgettext
 * uses for C/C++, and thus should be added explicitly with -k or
 * --keyword argument:
 *
 *     -kpgettext_expr:1c,2 -knpgettext_expr:1c,2,3
 *
 * Obviously, this file should only be included if you don't use
 * gettext.h. It's therefore is not a part of includes in dpso_utils.h.
 */

#pragma once


#if ENABLE_NLS

#include <libintl.h>

#else

#define gettext(msgId) msgId
#define ngettext(msgId1, msgId2, N) N == 1 ? msgId1 : msgId2
#define textdomain(domainName) (void)domainName
#define bindtextdomain(domainName, dirName) (void)dirName
#define bind_textdomain_codeset(domainName, codeset) (void)codeset

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
