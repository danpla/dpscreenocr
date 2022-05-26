
#pragma once

#include "backend/key_manager.h"


namespace dpso {
namespace backend {


class ActionExecutor;


class KeyManagerExecutor : public KeyManager {
public:
    KeyManagerExecutor(
        KeyManager& keyManager, ActionExecutor& actionExecutor);

    bool getHotkeysEnabled() const override;
    void setHotkeysEnabled(bool newHotkeysEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;
private:
    KeyManager& keyManager;
    ActionExecutor& actionExecutor;
};


}
}
