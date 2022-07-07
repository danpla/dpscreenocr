
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize libintl.
 *
 * The function performs all steps needed to set up libintl, such as
 * calling setlocale(), bindtextdomain(), etc.
 */
void uiInitIntl(void);


#ifdef __cplusplus
}
#endif
