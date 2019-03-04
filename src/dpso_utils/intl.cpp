
#include "intl.h"

#include <string>


static const char* getMsgCtxId(const char* msgCtx, const char* msgId)
{
    static std::string msgCtxId;

    msgCtxId.clear();
    msgCtxId += msgCtx;
    msgCtxId += '\004';
    msgCtxId += msgId;

    return msgCtxId.c_str();
}


const char* pgettext_expr(const char* msgCtx, const char* msgId)
{
    return pgettext_aux(getMsgCtxId(msgCtx, msgId), msgId);
}


const char* npgettext_expr(
    const char* msgCtx, const char* msgId1, const char* msgId2,
    unsigned long n)
{
    return npgettext_aux(
        getMsgCtxId(msgCtx, msgId1), msgId1, msgId2, n);
}
