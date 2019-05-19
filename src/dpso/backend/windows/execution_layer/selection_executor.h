
#pragma once

#include "backend/windows/windows_selection.h"
#include "backend/windows/execution_layer/action_executor.h"


namespace dpso {
namespace backend {


class SelectionExecutor : public Selection {
public:
    SelectionExecutor(
        WindowsSelection& selection, ActionExecutor& actionExecutor);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    Rect getGeometry() const override;

    void update();
private:
    WindowsSelection* selection;
    ActionExecutor* actionExecutor;
};


}
}
