
#pragma once

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/key_manager.h"


namespace dpso::backend {


class WindowsKeyManager : public KeyManager {
public:
    WindowsKeyManager() = default;
    ~WindowsKeyManager();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    void bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;

    void clearLastHotkeyAction();
    void handleWmHotkey(const MSG& msg);
private:
    bool isEnabled{};
    std::vector<HotkeyBinding> bindings;
    DpsoHotkeyAction hotkeyAction{dpsoNoHotkeyAction};

    HotkeyBinding* findBinding(const DpsoHotkey& hotkey);
};


}
