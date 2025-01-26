#pragma once

#include <QWidget>


class QTextEdit;


namespace ui::qt {


class About : public QWidget {
    Q_OBJECT
public:
    About();
signals:
    void checkUpdates();
private slots:
    void handleLinkActivation(const QString &link);
    void handleLinkHover(const QString &link);
private:
    QString currentTextLink;
    QTextEdit* textEdit;
};


}
