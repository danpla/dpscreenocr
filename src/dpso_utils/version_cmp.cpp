#include "version_cmp.h"

#include <cassert>
#include <charconv>
#include <cstddef>


namespace dpso {


VersionCmp::VersionCmp(std::string_view str)
{
    const auto* s = str.data();
    const auto* sEnd = s + str.size();

    std::size_t numCount{};
    while (s < sEnd) {
        NumT num;
        const auto [end, ec] = std::from_chars(s, sEnd, num);
        if (ec != std::errc{}) {
            if (s > str.data()) {
                // Make the preceding period a part of the extra
                // string.
                --s;
                assert(!nums.empty());
                assert(*s == '.');
            }

            break;
        }

        ++numCount;
        // We don't want trailing zeros so that "1" == "1.0.0"
        if (num != 0) {
            nums.resize(numCount);
            nums.back() = num;
        }

        s = end;

        if (s < sEnd && *s == '.')
            ++s;
        else
            break;
    }

    extra.assign(s, sEnd);
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
