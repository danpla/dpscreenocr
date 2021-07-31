
#pragma once

#include <QTreeWidget>


class QTreeWidgetItem;


class LangBrowser : public QTreeWidget {
    Q_OBJECT

public:
    explicit LangBrowser(QWidget* parent = nullptr);

    void loadState();
    void saveState() const;
private slots:
    void toggleLang(QTreeWidgetItem* item, int column);
};
