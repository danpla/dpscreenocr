
/**
 * \file Helpers for libintl.
 *
 * This file provides implementations of functions from gettext.h,
 * but only those we actually use.
 */

#pragma once


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
