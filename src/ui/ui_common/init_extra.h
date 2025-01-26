#pragma once


namespace ui {


// The initStart/End() functions are the place for additional
// platform-specific logic during uiInit(). initStart() is the first
// function called by uiInit(); initEnd() is the last. On failure,
// both should set an error message (dpsoGetError()) and return false.
bool initStart(int argc, char* argv[]);
bool initEnd(int argc, char* argv[]);


}
