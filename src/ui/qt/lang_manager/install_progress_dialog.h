
#pragma once

#include "dpso/dpso.h"

#include "lang_manager/install_mode.h"


class QWidget;


namespace ui::qt::langManager {


void runInstallProgressDialog(
    QWidget* parent,
    DpsoOcrLangManager* langManager,
    InstallMode installMode);


}
