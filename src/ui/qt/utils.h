#pragma once

#include <QFont>
#include <QIcon>
#include <QWidget>


QIcon getIcon(const QString &name);


bool confirmation(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText);


void setMonospace(QFont& font);
