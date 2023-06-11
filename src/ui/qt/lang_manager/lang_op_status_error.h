
#pragma once

#include "dpso/dpso.h"


class QString;
class QWidget;


namespace ui::qt::langManager {


// Does nothing if the status code is not an error.
void showLangOpStatusError(
    QWidget* parent,
    const QString& text,
    const DpsoOcrLangOpStatus& status);


}
