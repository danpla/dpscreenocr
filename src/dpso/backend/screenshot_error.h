#pragma once

#include "backend/backend_error.h"


namespace dpso::backend {


class ScreenshotError : public BackendError {
    using BackendError::BackendError;
};


}
