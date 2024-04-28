
#include "backend/windows/execution_layer/key_manager_executor.h"

#include "backend/windows/execution_layer/action_executor.h"


namespace dpso::backend {


#define EXECUTE(CALL) \
    execute(actionExecutor, [&]{ return keyManager.CALL; })


KeyManagerExecutor::KeyManagerExecutor(
        KeyManager& keyManager, ActionExecutor& actionExecutor)
    : keyManager{keyManager}
    , actionExecutor{actionExecutor}
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
