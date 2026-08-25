#pragma once

#include "backend/key_manager.h"


namespace dpso::backend {


class BgThreadExecutor;


class KeyManagerExecutor : public KeyManager {
public:
    KeyManagerExecutor(
        KeyManager& keyManager, BgThreadExecutor& bgThreadExecutor);

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    void bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;
private:
    KeyManager& keyManager;
    BgThreadExecutor& bgThreadExecutor;
};


}
