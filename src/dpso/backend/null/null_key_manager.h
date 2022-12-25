
#pragma once

#include <vector>

#include "backend/key_manager.h"


namespace dpso::backend {


class NullKeyManager : public KeyManager {
public:
    NullKeyManager();

    bool getHotkeysEnabled() const override;
    void setHotkeysEnabled(bool newHotkeysEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;
private:
    bool hotkeysEnabled;
    std::vector<HotkeyBinding> bindings;
};


}
