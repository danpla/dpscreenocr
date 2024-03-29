
#include "selection.h"

#include "backend/backend.h"
#include "backend/selection.h"


static dpso::backend::Selection* selection;


bool dpsoGetSelectionIsEnabled(void)
{
    return selection ? selection->getIsEnabled() : false;
}


void dpsoSetSelectionIsEnabled(bool newSelectionIsEnabled)
{
    if (selection)
        selection->setIsEnabled(newSelectionIsEnabled);
}


int dpsoGetSelectionDefaultBorderWidth(void)
{
    return dpso::backend::Selection::defaultBorderWidth;
}


void dpsoSetSelectionBorderWidth(int newBorderWidth)
{
    if (!selection)
        return;

    if (newBorderWidth < 1)
        newBorderWidth = 1;

    selection->setBorderWidth(newBorderWidth);
}


void dpsoGetSelectionGeometry(DpsoRect* rect)
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
