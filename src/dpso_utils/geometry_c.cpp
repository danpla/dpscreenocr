#include "geometry_c.h"


bool dpsoRectIsEmpty(const DpsoRect* rect)
{
    return !rect || rect->w <= 0 || rect->h <= 0;
}
