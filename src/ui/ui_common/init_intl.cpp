
#include "init_intl.h"

// On Windows, libintl patches setlocale with a macro that expands to
// libintl_setlocale. std::setlocale becomes std::libintl_setlocale,
// so we must use <locale.h> rather than <clocale>.
#include <locale.h>

#include "dpso_intl/dpso_bindtextdomain_utf8.h"
#include "dpso_intl/dpso_intl.h"

#include "app_dirs.h"
#include "file_names.h"


void uiInitIntl(void)
{
    setlocale(LC_ALL, "");

    bindtextdomainUtf8(uiAppFileName, uiGetAppDir(UiAppDirLocale));
    bind_textdomain_codeset(uiAppFileName, "UTF-8");
    textdomain(uiAppFileName);
}
