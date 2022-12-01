
#pragma once

#include "geometry.h"


namespace dpso {
namespace backend {


/**
 * Selection.
 *
 * See selection.h for more information.
 */
class Selection {
public:
    /**
     * Default border width at base (e.g. 96) DPI.
     */
    static const auto defaultBorderWidth = 3;

    /**
     * Dash length.
     *
     * The selection border is filled with a pattern of black and
     * white dashes. The dash length defines the number of squares in
     * a single dash.
     */
    static const auto dashLen = 3;

    virtual ~Selection() = default;

    /**
     * Get whether selection is enabled.
     *
     * \sa dpsoGetSelectionIsEnabled()
     */
    virtual bool getIsEnabled() const = 0;

    /**
     * Set whether selection is enabled.
     *
     * \sa dpsoSetSelectionIsEnabled()
     */
    virtual void setIsEnabled(bool newIsEnabled) = 0;

    /**
     * Set selection border width.
     *
     * The method is always called with newBorderWidth > 0.
     *
     * \sa dpsoSetSelectionBorderWidth()
     */
    virtual void setBorderWidth(int newBorderWidth) = 0;

    /**
     * Get selection geometry.
     *
     * \sa dpsoGetSelectionGeometry()
     */
    virtual Rect getGeometry() const = 0;
};


}
}
