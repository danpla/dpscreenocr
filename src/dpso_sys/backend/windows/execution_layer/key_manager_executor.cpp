#include "backend/windows/execution_layer/key_manager_executor.h"

#include "backend/windows/execution_layer/bg_thread_executor.h"


namespace dpso::backend {


#define EXECUTE(CALL) \
    bgThreadExecutor([&]{ return keyManager.CALL; })


KeyManagerExecutor::KeyManagerExecutor(
        KeyManager& keyManager, BgThreadExecutor& bgThreadExecutor)
    : keyManager{keyManager}
    , bgThreadExecutor{bgThreadExecutor}
{
}


bool KeyManagerExecutor::getIsEnabled() const
{
    return EXECUTE(getIsEnabled());
}


void KeyManagerExecutor::setIsEnabled(bool newIsEnabled)
{
    EXECUTE(setIsEnabled(newIsEnabled));
}


DpsoHotkeyAction KeyManagerExecutor::getLastHotkeyAction() const
{
    return EXECUTE(getLastHotkeyAction());
}


void KeyManagerExecutor::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    EXECUTE(bindHotkey(hotkey, action));
}


int KeyManagerExecutor::getNumBindings() const
{
    return EXECUTE(getNumBindings());
}


HotkeyBinding KeyManagerExecutor::getBinding(int idx) const
{
    return EXECUTE(getBinding(idx));
}


void KeyManagerExecutor::removeBinding(int idx)
{
    EXECUTE(removeBinding(idx));
}


}
