
#pragma once

#include <QIcon>


class QWidget;


QIcon getIcon(const QString &name);


bool confirmation(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText);


class QFont;


void setMonospace(QFont& font);
