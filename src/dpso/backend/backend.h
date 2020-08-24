
#pragma once

#include <memory>

#include "backend/error.h"


namespace dpso {


class Rect;


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
     * The rect should be clamped to screen.
     *
     * Returns null on errors. It's left for you to decide whether
     * an empty rectangle (after clamping to screen) is an error.
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
