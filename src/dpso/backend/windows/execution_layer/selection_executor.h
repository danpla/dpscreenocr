
#pragma once

#include "backend/selection.h"


namespace dpso {
namespace backend {


class ActionExecutor;


class SelectionExecutor : public Selection {
public:
    SelectionExecutor(
        Selection& selection, ActionExecutor& actionExecutor);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;
private:
    Selection& selection;
    ActionExecutor& actionExecutor;
};


}
}
