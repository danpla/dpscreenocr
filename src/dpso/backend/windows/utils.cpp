
#include "backend/windows/utils.h"


namespace dpso {
namespace backend {


std::string getLastErrorMessage()
{
    const auto error = GetLastError();
    if (error == ERROR_SUCCESS)
        return "";

    char* messageBuffer = nullptr;
    const auto size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<char*>(&messageBuffer),
        0, nullptr);

    if (size == 0)
        return "Windows error " + std::to_string(error);

    std::string message(messageBuffer, size);

    LocalFree(messageBuffer);

    return message;
}


}
}
