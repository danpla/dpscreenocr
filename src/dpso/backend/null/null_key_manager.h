
#pragma once

#include <vector>

#include "backend/backend.h"


namespace dpso {
namespace backend {


class NullKeyManager : public KeyManager {
public:
    NullKeyManager();

    bool getHotkeysEnabled() const override;
    void setHotkeysEnabled(bool newHotkeysEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    void getBinding(int idx, HotkeyBinding& hotkeyBinding) const override;
    void removeBinding(int idx) override;
private:
    bool hotkeysEnabled;
    std::vector<HotkeyBinding> bindings;
};


}
}
