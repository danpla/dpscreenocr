#include "autostart/autostart.h"

#include "autostart/autostart_windows_registry.h"
#include "msix_helper/msix_helper.h"


namespace ui {


std::unique_ptr<Autostart> Autostart::create(
    const Args& args)
{
    if (msix::isInMsix())
        return msix::createStartupTaskAutostart(args);

    return createRegistryAutostart(args);
}


}
