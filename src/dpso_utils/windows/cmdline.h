#pragma once

#include <cstddef>
#include <string>


namespace dpso::windows {


// Create a command line using CommandLineToArgv() rules.
//
// Note that on Windows the program name is parsed differently from
// the other parameters, and therefore comes as a separate programName
// argument in this function. If you need to build the command-line
// arguments without the program name (for example, to be used as
// lpCommandLine for CreateProcess() when lpApplicationName is not
// null), leave programName empty.
std::string createCmdLine(
    const char* programName,
    const char* const* args, std::size_t numArgs);


}
