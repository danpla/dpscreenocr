
#pragma once

#include "dpso/dpso.h"


class QWidget;


namespace langManager {


void runFetchLangsProgressDialog(
    const DpsoOcrLangManager* langManager,
    QWidget* parent);


}
