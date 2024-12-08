
#pragma once

#include "dpso/dpso.h"


class QWidget;


namespace ui::qt::langManager {


void runFetchLangsProgressDialog(
    QWidget* parent, const DpsoOcrLangManager* langManager);


}
