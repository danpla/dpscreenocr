
#include "get_data.h"

#include <cinttypes>
#include <limits>

#include "dpso_utils/str.h"

#include "error.h"
#include "request.h"


namespace dpso::net {


std::string getData(
    const char* url, const char* userAgent, std::int64_t sizeLimit)
{
    if (sizeLimit < 0)
        throw Error{"Size limit is < 0"};

    auto response = makeGetRequest(url, userAgent);

    if (const auto size = response->getSize();
            size && *size > sizeLimit)
        throw Error{str::printf(
            "Response data size (%" PRIi64 ") exceeds "
            "limit (%" PRIi64 ")",
            *size, sizeLimit)};

    std::string result;

    char buf[16 * 1024];
    while (true) {
        const auto numRead = response->read(buf, sizeof(buf));
        if (numRead == 0)
            break;

        if (std::numeric_limits<std::int64_t>::max() - result.size()
                    < numRead
                || static_cast<std::int64_t>(result.size() + numRead)
                    > sizeLimit)
            throw Error{str::printf(
                "Response data size exceeds limit (%" PRIi64 ")",
                sizeLimit)};

        result.append(buf, numRead);
    }

    return result;
}


}
