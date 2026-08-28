#pragma once

#include <memory>

#include "dpso_img/img.h"
#include "dpso_utils/geometry_fwd.h"


namespace dpso::backend {


class KeyManager;
class Selection;
class Screenshot;


class Backend {
public:
    // The concrete backend should provide definition of this method.
    // Throws BackendError.
    static std::unique_ptr<Backend> create();

    virtual ~Backend() = default;

    virtual KeyManager& getKeyManager() = 0;
    virtual Selection& getSelection() = 0;

    // The method will clamp the rect to screen. Throws
    // ScreenshotError.
    virtual img::ImgUPtr takeScreenshot(const Rect& rect) = 0;

    virtual void update() = 0;
};


}
