#pragma once

#include "autostart/autostart.h"


namespace ui::msix {


bool isInMsix();

bool isActivatedByStartupTask();

std::unique_ptr<Autostart> createStartupTaskAutostart(
    const Autostart::Args& args);


}
