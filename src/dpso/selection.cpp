#include "selection.h"

#include "backend/backend.h"
#include "backend/selection.h"


static dpso::backend::Selection* selection;


bool dpsoSelectionGetIsEnabled(void)
{
    return selection ? selection->getIsEnabled() : false;
}


void dpsoSelectionSetIsEnabled(bool newIsEnabled)
{
    if (selection)
        selection->setIsEnabled(newIsEnabled);
}


int dpsoSelectionGetDefaultBorderWidth(void)
{
    return dpso::backend::Selection::defaultBorderWidth;
}


void dpsoSelectionSetBorderWidth(int newBorderWidth)
{
    if (!selection)
        return;

    if (newBorderWidth < 1)
        newBorderWidth = 1;

    selection->setBorderWidth(newBorderWidth);
}


void dpsoSelectionGetGeometry(DpsoRect* rect)
{
    if (!rect)
        return;

    *rect =
        selection ? toCRect(selection->getGeometry()) : DpsoRect{};
}


namespace dpso::selection {


void init(dpso::backend::Backend& backend)
{
    ::selection = &backend.getSelection();
}


void shutdown()
{
    ::selection = nullptr;
}


}
