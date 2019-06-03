#pragma once

#include <QFont>
#include <QIcon>
#include <QWidget>


QIcon getIcon(const QString &name);


bool confirmation(
    QWidget* parent,
    const QString& text,
    const QString& cancelText,
    const QString& okText);


void setMonospace(QFont& font);
