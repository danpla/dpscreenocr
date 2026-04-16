#pragma once

#include "autostart/autostart.h"


namespace ui {


std::unique_ptr<Autostart> createRegistryAutostart(
    const Autostart::Args& args);


}
