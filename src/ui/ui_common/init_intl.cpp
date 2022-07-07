
#include "init_intl.h"

#include <string>

// On Windows, libintl patches setlocale with a macro that expands to
// libintl_setlocale. std::setlocale becomes std::libintl_setlocale,
// so we must use <locale.h> rather than <clocale>.
#include <locale.h>

#include "dpso_intl/dpso_bindtextdomain_utf8.h"
#include "dpso_intl/dpso_intl.h"

#include "dpso_utils/os.h"

#include "file_names.h"
#include "paths.h"


void uiInitIntl(void)
{
    setlocale(LC_ALL, "");

    const auto localeDirPath =
        std::string(uiGetBaseDirPath())
        + *dpsoDirSeparators
        + uiLocaleDir;

    bindtextdomainUtf8(uiAppFileName, localeDirPath.c_str());

    bind_textdomain_codeset(uiAppFileName, "UTF-8");
    textdomain(uiAppFileName);
}
