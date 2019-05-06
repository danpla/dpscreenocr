
#include "backend/null/null_key_manager.h"

#include <cassert>
#include <cstdio>

#include "hotkeys.h"


namespace dpso {
namespace backend {


#define MSG(FMT, ...) \
    std::printf("NullKeyManager: " FMT, __VA_ARGS__)


NullKeyManager::NullKeyManager()
    : hotkeysEnabled {}
    , bindings {}
{

}


bool NullKeyManager::getHotkeysEnabled() const
{
    return hotkeysEnabled;
}


void NullKeyManager::setHotkeysEnabled(bool newHotkeysEnabled)
{
    hotkeysEnabled = newHotkeysEnabled;
    MSG("Hotkeys %s\n", hotkeysEnabled ? "enabled" : "disabled");
}


DpsoHotkeyAction NullKeyManager::getLastHotkeyAction() const
{
    return -1;
}


bool NullKeyManager::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    MSG(
        "Bind hotkey %s; action %i\n",
        dpsoHotkeyToString(&hotkey), action);

    for (auto& binding : bindings)
        if (binding.hotkey.key == hotkey.key
                && binding.hotkey.mods == hotkey.mods) {
            binding.action = action;
            return true;
        }

    bindings.push_back({hotkey, action});
    return true;
}


int NullKeyManager::getNumBindings() const
{
    return bindings.size();
}


void NullKeyManager::getBinding(
    int idx, HotkeyBinding& hotkeyBinding) const
{
    assert(idx >= 0);
    assert(idx < static_cast<int>(bindings.size()));

    hotkeyBinding = bindings[idx];
    MSG(
        "Get binding %i (%s; action %i)\n",
        idx,
        dpsoHotkeyToString(&bindings[idx].hotkey),
        bindings[idx].action);
}


void NullKeyManager::removeBinding(int idx)
{
    assert(idx >= 0);
    assert(idx < static_cast<int>(bindings.size()));

    MSG(
        "Remove binding %i (%s; action %i)\n",
        idx,
        dpsoHotkeyToString(&bindings[idx].hotkey),
        bindings[idx].action);

    if (idx + 1 < static_cast<int>(bindings.size()))
        bindings[idx] = bindings.back();

    bindings.pop_back();
}


}
}
