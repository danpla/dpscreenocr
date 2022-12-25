
#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace dpso::windows {


std::string getErrorMessage(DWORD error);
std::string getHresultMessage(HRESULT hresult);


}
