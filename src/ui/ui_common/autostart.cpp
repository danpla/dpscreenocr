#include "autostart.h"

#include <vector>

#include "autostart/autostart.h"

#include "dpso_utils/error_set.h"


using namespace dpso;


struct UiAutostart {
    std::unique_ptr<ui::Autostart> impl;
};


UiAutostart* uiAutostartCreate(const UiAutostartArgs* args)
{
    if (!args) {
        setError("args is null");
        return {};
    }

    const std::vector<std::string_view> argsSv{
        args->args, args->args + args->numArgs};

    try {
        return new UiAutostart{
            ui::Autostart::create(
                {
                    args->appName,
                    args->appFileName,
                    argsSv.data(),
                    argsSv.size()})};
    } catch (ui::Autostart::Error& e) {
        setError("ui::Autostart::create(): {}", e.what());
        return {};
    }
}


void uiAutostartDelete(UiAutostart* autostart)
{
    delete autostart;
}


bool uiAutostartGetIsEnabled(const UiAutostart* autostart)
{
    return autostart && autostart->impl->getIsEnabled();
}


bool uiAutostartSetIsEnabled(
    UiAutostart* autostart, bool newIsEnabled)
{
    if (!autostart) {
        setError("autostart is null");
        return false;
    }

    try {
        autostart->impl->setIsEnabled(newIsEnabled);
        return true;
    } catch (ui::Autostart::Error& e) {
        setError("{}", e.what());
        return false;
    }
}
