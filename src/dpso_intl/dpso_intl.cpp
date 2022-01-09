
#include "dpso_intl.h"

#include <string>


static const char* getMsgCtxId(const char* msgCtx, const char* msgId)
{
    static std::string msgCtxId;

    msgCtxId = msgCtx;
    msgCtxId += '\004';
    msgCtxId += msgId;

    return msgCtxId.c_str();
}


const char* pgettext_aux(const char* msgCtxId, const char* msgId)
{
    const char* translated = gettext(msgCtxId);
    return translated == msgCtxId ? msgId : translated;
}


const char* pgettext_expr(const char* msgCtx, const char* msgId)
{
    return pgettext_aux(getMsgCtxId(msgCtx, msgId), msgId);
}


const char* npgettext_aux(
    const char* msgCtxId, const char* msgId1, const char* msgId2,
    unsigned long n)
{
    const char* translated = ngettext(msgCtxId, msgId2, n);
    if (translated == msgCtxId || translated == msgId2)
        return n == 1 ? msgId1 : msgId2;
    else
        return translated;
}


const char* npgettext_expr(
    const char* msgCtx, const char* msgId1, const char* msgId2,
    unsigned long n)
{
    return npgettext_aux(
        getMsgCtxId(msgCtx, msgId1), msgId1, msgId2, n);
}
