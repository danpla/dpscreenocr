
#include "selection.h"

#include "backend/backend.h"
#include "backend/selection.h"


static dpso::backend::Selection* selection;


int dpsoGetSelectionIsEnabled(void)
{
    return selection ? selection->getIsEnabled() : false;
}


void dpsoSetSelectionIsEnabled(int newSelectionIsEnabled)
{
    if (selection)
        selection->setIsEnabled(newSelectionIsEnabled);
}


void dpsoGetSelectionGeometry(DpsoRect* rect)
{
    if (!rect)
        return;

    *rect =
        selection ? toCRect(selection->getGeometry()) : DpsoRect{};
}


namespace dpso {
namespace selection {


void init(dpso::backend::Backend& backend)
{
    ::selection = &backend.getSelection();
}


void shutdown()
{
    ::selection = nullptr;
}


}
}
