#pragma once

#include <vector>


namespace dpso::sound {


struct AudioData {
    int numChannels;
    int rate;
    std::vector<short> samples;
};


}
