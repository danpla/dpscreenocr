#pragma once

#include "backend/backend.h"


namespace dpso::backend::x11 {


std::unique_ptr<Backend> createBackend();


}
