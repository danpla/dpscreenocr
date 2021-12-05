
#pragma once

#include "backend/backend_error.h"


namespace dpso {
namespace backend {


class ScreenshotError : public BackendError {
    using BackendError::BackendError;
};


}
}
