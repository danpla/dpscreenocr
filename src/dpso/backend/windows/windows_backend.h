
#pragma once

#include "backend/backend.h"


namespace dpso {
namespace backend {


class WindowsBackend : public Backend {
public:
    static Backend* create();
private:
    WindowsBackend();
};


}
}
