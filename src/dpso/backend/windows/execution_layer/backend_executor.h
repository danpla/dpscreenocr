
#pragma once

#include <memory>

#include "backend/backend.h"
#include "backend/windows/execution_layer/action_executor.h"


namespace dpso {
namespace backend {


using BackendCreatorFn = std::unique_ptr<Backend> (&)();
std::unique_ptr<Backend> createBackendExecutor(
    std::unique_ptr<ActionExecutor> actionExecutor,
    BackendCreatorFn creatorFn);


}
}
