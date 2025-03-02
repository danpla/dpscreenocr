#pragma once

#include <functional>

#include "ui_common/ui_common.h"

#include <QIcon>


class QFont;
class QMargins;
class QWidget;


// Formatting support for ui::strNFormat().
std::string toStr(const QString& s);


namespace ui::qt {


QString strNFormat(
    const char* str, std::initializer_list<ui::StrNFormatArg> args);


QString formatDataSize(qint64 size);


QString joinInLayoutDirection(
    const QString& separator, const QStringList& list);


QMargins makeSubordinateControlMargins();


QIcon getIcon(const QString &name);


// The function is the same as getIcon(), except that on Unix-like
// systems it first tries to load the icon from the current theme.
QIcon getThemeIcon(const QString &name);


// Display a progress dialog that will be closed when the callback
// returns false.
void showProgressDialog(
    QWidget* parent,
    const QString& text,
    const std::function<bool()>& callback);


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
