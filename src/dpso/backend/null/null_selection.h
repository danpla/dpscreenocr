
#pragma once

#include "backend/backend.h"


namespace dpso {
namespace backend {


class NullSelection : public Selection {
public:
    NullSelection();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newEnabled) override;

    Rect getGeometry() const override;
private:
    bool isEnabled;
};


}
}
