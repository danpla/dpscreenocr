#include "lang_manager/lang_op_status_error.h"

#include <QString>

#include "ui_common/ui_common.h"

#include "utils.h"


#define _(S) uiTranslate(S)


namespace ui::qt::langManager {


void showLangOpStatusError(
    QWidget* parent,
    const QString& text,
    const DpsoOcrLangOpStatus& status)
{
    QString informativeText;

    switch (status.code) {
    case DpsoOcrLangOpStatusCodeNone:
    case DpsoOcrLangOpStatusCodeProgress:
    case DpsoOcrLangOpStatusCodeSuccess:
        return;
    case DpsoOcrLangOpStatusCodeGenericError:
        break;
    case DpsoOcrLangOpStatusCodeNetworkConnectionError:
        informativeText =
            _("Check your network connection and try again.");
        break;
    }

    showError(parent, text, informativeText, status.errorText);
}


}
