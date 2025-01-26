#pragma once


namespace ui {


// Return a value of the LAUNCHER_ARGV0 environment variable, or argv0
// if LAUNCHER_ARGV0 either not set or empty.
const char* getToplevelArgv0(const char* argv0);


}
