#pragma once

#include <string_view>
#include <vector>

#include "plural/token.h"


namespace dp::intl::plural {


std::vector<Token> tokenize(std::string_view s);


}
