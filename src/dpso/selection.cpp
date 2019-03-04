
#include "selection.h"

#include "backend/backend.h"


static inline dpso::backend::Selection& getSelection()
{
    return dpso::backend::getBackend().getSelection();
}


int dpsoGetSelectionIsEnabled(void)
{
    return getSelection().getIsEnabled();
}


void dpsoSetSelectionIsEnabled(int newSelectionIsEnabled)
{
    getSelection().setIsEnabled(newSelectionIsEnabled);
}


void dpsoGetSelectionGeometry(int* x, int* y, int* w, int* h)
{
    const auto geom = getSelection().getGeometry();

    if (x)
        *x = geom.x;
    if (y)
        *y = geom.y;
    if (w)
        *w = geom.w;
    if (h)
        *h = geom.h;
}
