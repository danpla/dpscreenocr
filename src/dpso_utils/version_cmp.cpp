#include "version_cmp.h"

#include <cassert>
#include <charconv>
#include <cstring>


namespace dpso {


VersionCmp::VersionCmp(const char* str)
{
    const auto* s = str;
    const auto* sEnd = s + std::strlen(s);

    while (s < sEnd) {
        // Use unsigned so that from_chars() doesn't accept the minus
        // sign.
        unsigned num;
        const auto [end, ec] = std::from_chars(s, sEnd, num);
        if (ec != std::errc{}) {
            if (s > str) {
                // Make the preceding period a part of the extra
                // string.
                --s;
                assert(!nums.empty());
                assert(*s == '.');
            }

            break;
        }

        nums.push_back(num);

        s = end;

        if (*s == '.')
            ++s;
        else
            break;
    }

    extra = s;
}


bool VersionCmp::operator<(const VersionCmp& other) const
{
    if (nums < other.nums)
        return true;

    if (nums > other.nums)
        return false;

    // 1.0-rc1 < 1.0
    if (!extra.empty() && other.extra.empty())
        return true;

    // 1.0 > 1.0-rc1
    if (extra.empty() && !other.extra.empty())
        return false;

    // 1.0-rc1 < 1.0-rc2
    return extra < other.extra;
}


}
