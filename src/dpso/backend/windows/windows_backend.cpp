
#include "backend/windows/windows_backend.h"

#include "backend/null/null_screenshot.h"


namespace dpso {
namespace backend {


WindowsBackend::WindowsBackend()
{
    keyManager.reset(new NullKeyManager());
    selection.reset(new NullSelection());
}

KeyManager& WindowsBackend::getKeyManager()
{
    return *keyManager;
}


Selection& WindowsBackend::getSelection()
{
    return *selection;
}


Screenshot* WindowsBackend::takeScreenshot(const Rect& rect)
{
    return new NullScreenshot(rect);
}


void WindowsBackend::update()
{

}


}
}
