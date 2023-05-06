
#include "version_cmp.h"

#include <cassert>
#include <cstdlib>


namespace dpso {


VersionCmp::VersionCmp()
    : VersionCmp{""}
{
}


VersionCmp::VersionCmp(const char* versionStr)
{
    const auto* s = versionStr;

    while (*s) {
        // We don't need optional spaces, -, and + accepted by strtol.
        if (*s < '0' || *s > '9')
            break;

        char* end;
        const auto num = std::strtol(s, &end, 10);
        if (end == s) {
            if (s > versionStr) {
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
