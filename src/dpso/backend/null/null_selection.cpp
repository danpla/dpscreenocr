
#include "backend/null/null_selection.h"

#include <cstdio>


namespace dpso {
namespace backend {


#define MSG(...) std::printf("NullSelection: " __VA_ARGS__)


NullSelection::NullSelection()
    : isEnabled{}
{

}


bool NullSelection::getIsEnabled() const
{
    return isEnabled;
}


void NullSelection::setIsEnabled(bool newEnabled)
{
    isEnabled = newEnabled;
    MSG("Selection %s\n", isEnabled ? "enabled" : "disabled");
}


Rect NullSelection::getGeometry() const
{
    return {0, 0, 16, 16};
}


}
}
