
#pragma once

#include <memory>

#include "backend/error.h"


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
     * A concrete backend should provide definition of this method.
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
     * The method will clamp the rect to screen. If the clamped rect
     * is empty, returns either null or a screenshot with empty size,
     * depending on the implementation.
     *
     * Returns null on errors.
     */
    virtual std::unique_ptr<Screenshot> takeScreenshot(
        const Rect& rect) = 0;

    virtual void update() = 0;
};


void init();
void shutdown();
Backend& getBackend();


}
}
