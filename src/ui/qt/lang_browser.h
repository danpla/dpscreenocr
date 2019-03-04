
#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>


class LangBrowser : public QTreeWidget {
    Q_OBJECT

public:
    explicit LangBrowser(QWidget* parent = nullptr);

    void loadState();
    void saveState() const;
private slots:
    void toggleLang(QTreeWidgetItem* item);
private:
    enum ColumnIdx {
        columnIdxCheckbox,
        columnIdxName,
        columnIdxCode
    };
};
