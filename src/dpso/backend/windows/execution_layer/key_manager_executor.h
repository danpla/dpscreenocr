
#pragma once

#include "backend/windows/windows_key_manager.h"
#include "backend/windows/execution_layer/action_executor.h"


namespace dpso {
namespace backend {


class KeyManagerExecutor : public KeyManager {
public:
    KeyManagerExecutor(
        WindowsKeyManager& keyManager, ActionExecutor& actionExecutor);

    bool getHotkeysEnabled() const override;
    void setHotkeysEnabled(bool newHotkeysEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;

    void clearLastHotkeyAction();
    void handleWmHotkey(const MSG& msg);
private:
    WindowsKeyManager* keyManager;
    ActionExecutor* actionExecutor;
};


}
}
