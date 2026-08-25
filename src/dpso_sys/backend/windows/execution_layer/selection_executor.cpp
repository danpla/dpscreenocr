#include "backend/windows/execution_layer/selection_executor.h"

#include "backend/windows/execution_layer/bg_thread_executor.h"


namespace dpso::backend {


#define EXECUTE(CALL) \
    bgThreadExecutor([&]{ return selection.CALL; })


SelectionExecutor::SelectionExecutor(
        Selection& selection, BgThreadExecutor& bgThreadExecutor)
    : selection{selection}
    , bgThreadExecutor{bgThreadExecutor}
{
}


bool SelectionExecutor::getIsEnabled() const
{
    return EXECUTE(getIsEnabled());
}


void SelectionExecutor::setIsEnabled(bool newIsEnabled)
{
    EXECUTE(setIsEnabled(newIsEnabled));
}


void SelectionExecutor::setBorderWidth(int newBorderWidth)
{
    EXECUTE(setBorderWidth(newBorderWidth));
}


Rect SelectionExecutor::getGeometry() const
{
    return EXECUTE(getGeometry());
}


}
