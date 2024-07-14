
#pragma once

#include <QIcon>


class QFont;
class QMargins;
class QWidget;


namespace ui::qt {


QString formatDataSize(qint64 size);


QString joinInLayoutDirection(
    const QString& separator, const QStringList& list);


QMargins makeSubordinateControlMargins();


QIcon getIcon(const QString &name);


// The function is the same as getIcon(), except that on Unix-like
// systems it first tries to load the icon from the current theme.
QIcon getThemeIcon(const QString &name);


bool confirmDestructiveAction(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText);


void showError(
    QWidget* parent,
    const QString& text,
    const QString& informativeText = {},
    const QString& detailedText = {});


void setMonospace(QFont& font);


}
