#pragma once


namespace ui {


// Initialize libintl.
//
// The function performs all steps needed to set up libintl, such as
// calling setlocale(), bindtextdomain(), etc. It depends on
// UiAppDirLocale, so uiInitAppDirs() must be called before use.
void initIntl();


}
