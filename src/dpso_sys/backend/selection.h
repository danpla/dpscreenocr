#pragma once

#include "dpso_utils/geometry.h"


namespace dpso::backend {


// See selection.h for more information.
class Selection {
public:
    // Default border width at base (e.g. 96) DPI.
    static const auto defaultBorderWidth = 3;

    // The selection border is filled with a pattern of black and
    // white dashes. The dash length defines the number of squares in
    // a single dash.
    static const auto dashLen = 3;

    virtual ~Selection() = default;

    // See dpsoSelectionGetIsEnabled()
    virtual bool getIsEnabled() const = 0;

    // See dpsoSelectionSetIsEnabled()
    virtual void setIsEnabled(bool newIsEnabled) = 0;

    // See dpsoSelectionSetBorderWidth()
    //
    // The method is always called with newBorderWidth > 0.
    virtual void setBorderWidth(int newBorderWidth) = 0;

    // See dpsoSelectionGetGeometry()
    virtual Rect getGeometry() const = 0;
};


}
