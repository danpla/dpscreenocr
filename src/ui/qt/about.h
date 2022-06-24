
#pragma once

#include <QWidget>


class QTextEdit;


class About : public QWidget {
    Q_OBJECT

public:
    explicit About(QWidget* parent = nullptr);
private slots:
    void handleLinkActivation(const QString &link);
    void handleLinkHover(const QString &link);
private:
    QString currentTextLink;
    QTextEdit* textEdit;
};
