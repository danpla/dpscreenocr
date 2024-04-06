
#include "windows/error.h"

#include "str.h"


namespace dpso::windows {


std::string getErrorMessage(DWORD error, HMODULE module)
{
    char* messageBuf{};
    auto size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | (module ? FORMAT_MESSAGE_FROM_HMODULE : 0)
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        module,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<char*>(&messageBuf),
        0, nullptr);

    if (size == 0)
        return str::format("Windows error {}", error);

    if (size > 1
            && messageBuf[size - 2] == '\r'
            && messageBuf[size - 1] == '\n')
        size -= 2;

    std::string message{messageBuf, size};

    LocalFree(messageBuf);

    return message;
}


std::string getHresultMessage(HRESULT hresult)
{
    // It seems that FormatMessage() accepts HRESULT, at least
    // _com_error::ErrorMessage() relies on that. Still, this is not
    // documented, so we extract the system error code manually.
    if (HRESULT_FACILITY(hresult) == FACILITY_WIN32)
        return getErrorMessage(HRESULT_CODE(hresult));

    return
        "HRESULT 0x"
        + str::rightJustify(str::toStr(hresult, 16), 8, '0');
}


}
