#pragma once

#include <string>
#include <vector>


namespace dpso {


// A helper to compare version strings.
//
// A version string can contain zero or more numbers separated by
// periods, and an optional trailing text. Leading zeros in numbers
// are ignored. Trailing texts are compared lexicographically, but a
// version with extra text is considered to be less than the same
// version without extra text. For example:
//
//   1.0-rc1 < 1.0-rc2 < 1.0
class VersionCmp {
public:
    VersionCmp() = default;
    explicit VersionCmp(const char* str);

    bool operator<(const VersionCmp& other) const;
private:
    std::vector<unsigned> nums;
    std::string extra;
};


}
