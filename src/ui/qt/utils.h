
#pragma once

#include <QIcon>


class QWidget;


QString joinInLayoutDirection(
    const QString& separator, const QStringList& list);


QIcon getIcon(const QString &name);


// The function is the same as getIcon(), except that on Unix-like
// systems it first tries to load the icon from the current theme.
QIcon getThemeIcon(const QString &name);


// Qt 4 has a nasty bug when QIcon::pixmap() for an icon loaded via
// QIcon::fromTheme() returns a pixmap bigger than the requested size.
// This function detects this case and scales the pixmap down.
QPixmap getPixmap(
    const QIcon& icon,
    int size,
    QIcon::Mode mode = QIcon::Normal,
    QIcon::State state = QIcon::Off);


bool confirmation(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText);


class QFont;


void setMonospace(QFont& font);
