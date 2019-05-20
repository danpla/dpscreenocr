
#pragma once

#include <cstdint>
#include <stdexcept>

#include "geometry.h"
#include "types.h"


namespace dpso {
namespace backend {


class BackendError : public std::runtime_error {
    using runtime_error::runtime_error;
};


struct HotkeyBinding {
    DpsoHotkey hotkey;
    DpsoHotkeyAction action;
};


/**
 * Key manager.
 *
 * See hotkeys.h for more information.
 *
 * Preconditions:
 *
 *   * bindHotkey():
 *       * hotkey.key >= 0 and < dpsoNumKeys
 *       * action >= 0
 *   * getBinding(), removeBinding():
 *       * idx >= 0 and < getNumBindings()
 */
class KeyManager {
public:
    KeyManager() = default;
    virtual ~KeyManager() {};

    KeyManager(const KeyManager& other) = delete;
    KeyManager& operator=(const KeyManager& other) = delete;

    KeyManager(KeyManager&& other) = delete;
    KeyManager& operator=(KeyManager&& other) = delete;

    virtual bool getHotkeysEnabled() const = 0;
    virtual void setHotkeysEnabled(bool newHotkeysEnabled) = 0;
    virtual DpsoHotkeyAction getLastHotkeyAction() const = 0;

    virtual bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) = 0;

    virtual int getNumBindings() const = 0;
    virtual void getBinding(
        int idx, HotkeyBinding& hotkeyBinding) const = 0;
    virtual void removeBinding(int idx) = 0;
};


/**
 * Selection.
 *
 * See selection.h for more information.
 */
class Selection {
public:
    Selection() = default;
    virtual ~Selection() {};

    Selection(const Selection& other) = delete;
    Selection& operator=(const Selection& other) = delete;

    Selection(Selection&& other) = delete;
    Selection& operator=(Selection&& other) = delete;

    virtual bool getIsEnabled() const = 0;
    virtual void setIsEnabled(bool newIsEnabled) = 0;

    virtual Rect getGeometry() const = 0;
};


class Screenshot {
public:
    Screenshot() = default;
    virtual ~Screenshot() {};

    Screenshot(const Screenshot& other) = delete;
    Screenshot& operator=(const Screenshot& other) = delete;

    Screenshot(Screenshot&& other) = delete;
    Screenshot& operator=(Screenshot&& other) = delete;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void getGrayscaleData(std::uint8_t* buf, int pitch) const = 0;
};


class Backend {
public:
    Backend() = default;
    virtual ~Backend() {};

    Backend(const Backend& other) = delete;
    Backend& operator=(const Backend& other) = delete;

    Backend(Backend&& other) = delete;
    Backend& operator=(Backend&& other) = delete;

    virtual KeyManager& getKeyManager() = 0;
    virtual Selection& getSelection() = 0;
    virtual Screenshot* takeScreenshot(const Rect& rect) = 0;

    virtual void update() = 0;
};


void init();
void shutdown();
Backend& getBackend();


}
}
