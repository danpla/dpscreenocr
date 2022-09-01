
#include "backend/windows/execution_layer/selection_executor.h"

#include "backend/windows/execution_layer/action_executor.h"


namespace dpso {
namespace backend {


#define EXECUTE(CALL) \
    execute(actionExecutor, [&](){ return selection.CALL; })


SelectionExecutor::SelectionExecutor(
        Selection& selection, ActionExecutor& actionExecutor)
    : selection{selection}
    , actionExecutor{actionExecutor}
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
}
