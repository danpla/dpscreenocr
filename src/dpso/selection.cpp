
#include "selection.h"

#include "backend/backend.h"
#include "backend/selection.h"


static dpso::backend::Selection& getSelection()
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


void dpsoGetSelectionGeometry(struct DpsoRect* rect)
{
    if (rect)
        *rect = toCRect(getSelection().getGeometry());
}
