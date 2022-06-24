
#pragma once

#include <QIcon>


class QWidget;


QIcon getIcon(const QString &name);

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
