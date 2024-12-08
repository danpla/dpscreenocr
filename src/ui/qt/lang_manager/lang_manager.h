
#pragma once

#include <string>


class QWidget;


namespace ui::qt::langManager {


void runLangManager(
    QWidget* parent, int ocrEngineIdx, const std::string& dataDir);


}
