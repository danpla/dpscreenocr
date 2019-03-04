#pragma once

#include <QIcon>
#include <QWidget>


QIcon getIcon(const QString &name);

bool confirmation(
    QWidget* parent,
    const QString& text,
    const QString& cancelText,
    const QString& okText);
