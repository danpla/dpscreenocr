
#pragma once

#include <memory>


namespace dpso {


struct Rect;


namespace backend {


class KeyManager;
class Selection;
class Screenshot;


class Backend {
public:
    /**
     * Create a backend.
     *
     * The concrete backend should provide definition of this method.
     *
     * \throws BackendError
     */
    static std::unique_ptr<Backend> create();

    Backend() = default;
    virtual ~Backend() = default;

    Backend(const Backend& other) = delete;
    Backend& operator=(const Backend& other) = delete;

    Backend(Backend&& other) = delete;
    Backend& operator=(Backend&& other) = delete;

    virtual KeyManager& getKeyManager() = 0;
    virtual Selection& getSelection() = 0;

    /**
     * Take screenshot.
     *
     * The method will clamp the rect to screen.
     *
     * \throws ScreenshotError
     */
    virtual std::unique_ptr<Screenshot> takeScreenshot(
        const Rect& rect) = 0;

    virtual void update() = 0;
};


}
}
