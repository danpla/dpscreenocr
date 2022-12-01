
#pragma once

#include <QIcon>


class QWidget;


QString joinInLayoutDirection(
    const QString& separator, const QStringList& list);


QIcon getIcon(const QString &name);


// The function is the same as getIcon(), except that on Unix-like
// systems it first tries to load the icon from the current theme.
QIcon getThemeIcon(const QString &name);


bool confirmDestructiveAction(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText);


class QFont;


void setMonospace(QFont& font);
