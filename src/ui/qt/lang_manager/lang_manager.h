
#pragma once

#include <string>


class QWidget;


namespace ui::qt::langManager {


void runLangManager(
    int ocrEngineIdx, const std::string& dataDir, QWidget* parent);


}
