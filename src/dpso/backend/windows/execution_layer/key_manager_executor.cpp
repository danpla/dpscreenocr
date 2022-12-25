
#include "backend/windows/execution_layer/key_manager_executor.h"

#include "backend/windows/execution_layer/action_executor.h"


namespace dpso::backend {


#define EXECUTE(CALL) \
    execute(actionExecutor, [&](){ return keyManager.CALL; })


KeyManagerExecutor::KeyManagerExecutor(
        KeyManager& keyManager, ActionExecutor& actionExecutor)
    : keyManager{keyManager}
    , actionExecutor{actionExecutor}
{
}


bool KeyManagerExecutor::getHotkeysEnabled() const
{
    return EXECUTE(getHotkeysEnabled());
}


void KeyManagerExecutor::setHotkeysEnabled(bool newHotkeysEnabled)
{
    EXECUTE(setHotkeysEnabled(newHotkeysEnabled));
}


DpsoHotkeyAction KeyManagerExecutor::getLastHotkeyAction() const
{
    return EXECUTE(getLastHotkeyAction());
}


bool KeyManagerExecutor::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    return EXECUTE(bindHotkey(hotkey, action));
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
