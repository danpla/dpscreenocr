
#include "backend/windows/execution_layer/selection_executor.h"


namespace dpso {
namespace backend {


#define EXEC_DELEGATE(CALL) \
    execute(*actionExecutor, [&](){ return selection->CALL; })


SelectionExecutor::SelectionExecutor(
        Selection& selection, ActionExecutor& actionExecutor)
    : selection {&selection}
    , actionExecutor {&actionExecutor}
{

}


bool SelectionExecutor::getIsEnabled() const
{
    return EXEC_DELEGATE(getIsEnabled());
}


void SelectionExecutor::setIsEnabled(bool newIsEnabled)
{
    EXEC_DELEGATE(setIsEnabled(newIsEnabled));
}


Rect SelectionExecutor::getGeometry() const
{
    return EXEC_DELEGATE(getGeometry());
}


}
}
