#pragma once

#include "backend/selection.h"


namespace dpso::backend {


class BgThreadExecutor;


class SelectionExecutor : public Selection {
public:
    SelectionExecutor(
        Selection& selection, BgThreadExecutor& bgThreadExecutor);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;

    void setBorderWidth(int newBorderWidth) override;

    Rect getGeometry() const override;
private:
    Selection& selection;
    BgThreadExecutor& bgThreadExecutor;
};


}
