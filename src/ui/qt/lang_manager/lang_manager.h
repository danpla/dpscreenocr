
#pragma once

#include <string>


class QWidget;


namespace langManager {


void runLangManager(
    int ocrEngineIdx, const std::string& dataDir, QWidget* parent);


}
