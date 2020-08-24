
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
    Selection() = default;
    virtual ~Selection() = default;

    Selection(const Selection& other) = delete;
    Selection& operator=(const Selection& other) = delete;

    Selection(Selection&& other) = delete;
    Selection& operator=(Selection&& other) = delete;

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
     * Get selection geometry.
     *
     * \sa dpsoGetSelectionGeometry()
     */
    virtual Rect getGeometry() const = 0;
};


}
}
