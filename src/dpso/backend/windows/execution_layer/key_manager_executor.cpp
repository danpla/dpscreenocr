
#include "backend/windows/execution_layer/key_manager_executor.h"


namespace dpso {
namespace backend {


#define EXEC_DELEGATE(CALL) \
    execute(*actionExecutor, [&](){ return keyManager->CALL; })


KeyManagerExecutor::KeyManagerExecutor(
        KeyManager& keyManager, ActionExecutor& actionExecutor)
    : keyManager {&keyManager}
    , actionExecutor {&actionExecutor}
{

}


bool KeyManagerExecutor::getHotkeysEnabled() const
{
    return EXEC_DELEGATE(getHotkeysEnabled());
}


void KeyManagerExecutor::setHotkeysEnabled(bool newHotkeysEnabled)
{
    EXEC_DELEGATE(setHotkeysEnabled(newHotkeysEnabled));
}


DpsoHotkeyAction KeyManagerExecutor::getLastHotkeyAction() const
{
    return EXEC_DELEGATE(getLastHotkeyAction());
}


bool KeyManagerExecutor::bindHotkey(
    const DpsoHotkey& hotkey, DpsoHotkeyAction action)
{
    return EXEC_DELEGATE(bindHotkey(hotkey, action));
}


int KeyManagerExecutor::getNumBindings() const
{
    return EXEC_DELEGATE(getNumBindings());
}


HotkeyBinding KeyManagerExecutor::getBinding(int idx) const
{
    return EXEC_DELEGATE(getBinding(idx));
}


void KeyManagerExecutor::removeBinding(int idx)
{
    EXEC_DELEGATE(removeBinding(idx));
}


}
}
