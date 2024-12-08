
#pragma once

#include "ui_common/ui_common.h"


class QWidget;


namespace ui::qt {


void runUpdateCheckProgressDialog(
    const UiUpdateChecker* updateChecker, QWidget* parent);


// Does nothing if the code is not an error.
void showUpdateCheckError(
    QWidget* parent, UiUpdateCheckerStatus status);


void showUpdateInfo(
    QWidget* parent, const UiUpdateCheckerUpdateInfo& updateInfo);


}
