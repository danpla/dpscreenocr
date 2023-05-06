
#pragma once

#include "dpso/dpso.h"

#include "lang_manager/install_mode.h"


class QWidget;


namespace langManager {


void runInstallProgressDialog(
    DpsoOcrLangManager* langManager,
    InstallMode installMode,
    QWidget* parent);


}
