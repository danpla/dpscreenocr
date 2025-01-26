#include "helpers.h"

#include <string>

#include <libintl.h>


static std::string getMsgCtxId(const char* msgCtx, const char* msgId)
{
    return std::string{msgCtx} + '\004' + msgId;
}


const char* pgettext_aux(const char* msgCtxId, const char* msgId)
{
    const auto* translated = gettext(msgCtxId);
    return translated == msgCtxId ? msgId : translated;
}


const char* pgettext_expr(const char* msgCtx, const char* msgId)
{
    return pgettext_aux(getMsgCtxId(msgCtx, msgId).c_str(), msgId);
}


const char* npgettext_aux(
    const char* msgCtxId, const char* msgId1, const char* msgId2,
    unsigned long n)
{
    const auto* translated = ngettext(msgCtxId, msgId2, n);
    if (translated == msgCtxId || translated == msgId2)
        return n == 1 ? msgId1 : msgId2;

    return translated;
}


const char* npgettext_expr(
    const char* msgCtx, const char* msgId1, const char* msgId2,
    unsigned long n)
{
    return npgettext_aux(
        getMsgCtxId(msgCtx, msgId1).c_str(), msgId1, msgId2, n);
}
