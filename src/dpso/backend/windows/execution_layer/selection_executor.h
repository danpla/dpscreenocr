
#pragma once

#include "backend/selection.h"
#include "backend/windows/execution_layer/action_executor.h"


namespace dpso {
namespace backend {


class SelectionExecutor : public Selection {
public:
    SelectionExecutor(
        Selection& selection, ActionExecutor& actionExecutor);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    Rect getGeometry() const override;
private:
    Selection* selection;
    ActionExecutor* actionExecutor;
};


}
}
