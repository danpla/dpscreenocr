
#pragma once

#include "dpso/dpso.h"


class QWidget;


namespace ui::qt::langManager {


void runFetchLangsProgressDialog(
    const DpsoOcrLangManager* langManager,
    QWidget* parent);


}
