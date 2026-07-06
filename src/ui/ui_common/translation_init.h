#pragma once


namespace ui {


// Initialize translations.
//
// The function tries to pick the message catalog (MO file) that best
// matches one of the languages from the system settings. Before
// performing any action, the function resets the current language to
// English ("en"), which will effectively remain active if no suitable
// match is found or in case of an error.
//
// initTranslation() depends on UiAppDirData, which in turn depends on
// the executable path, so the function must be called after
// initExePath().
//
// On failure, sets an error message (dpsoGetError()) and returns
// false.
bool initTranslation();


}
