#include "selection.h"

#include "backend/selection.h"
#include "dpso_sys_p.h"


bool dpsoSelectionGetIsEnabled(const DpsoSelection* selection)
{
    return selection && selection->impl.getIsEnabled();
}


void dpsoSelectionSetIsEnabled(
    DpsoSelection* selection, bool newIsEnabled)
{
    if (selection)
        selection->impl.setIsEnabled(newIsEnabled);
}


int dpsoSelectionGetDefaultBorderWidth(void)
{
    return dpso::backend::Selection::defaultBorderWidth;
}


void dpsoSelectionSetBorderWidth(
    DpsoSelection* selection, int newBorderWidth)
{
    if (selection)
        selection->impl.setBorderWidth(
            newBorderWidth < 1 ? 1 : newBorderWidth);
}


void dpsoSelectionGetGeometry(
    const DpsoSelection* selection, DpsoRect* rect)
{
    if (!rect)
        return;

    *rect = selection
        ? toCRect(selection->impl.getGeometry()) : DpsoRect{};
}
