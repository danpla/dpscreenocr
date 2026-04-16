#pragma once


namespace ui {


// Initialize libintl.
//
// The function performs all steps needed to set up libintl, such as
// calling setlocale(), bindtextdomain(), etc. It depends on
// UiAppDirLocale, which in turn depends on the executable path, so
// must be called after initExePath().
void initIntl();


}
