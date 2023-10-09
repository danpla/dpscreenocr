
#pragma once

#include <string>

#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QWidget>

#include "dpso_ext/dpso_ext.h"


class QPushButton;
class QTextEdit;


namespace ui::qt {


class History : public QWidget {
    Q_OBJECT
public:
    explicit History(const std::string& dirPath);

    void append(const char* timestamp, const char* text);

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private slots:
    void doExport();
    void clear();
private:
    std::string historyFilePath;
    dpso::HistoryUPtr history;

    bool wrapWords{};

    QTextEdit* textEdit;
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    int blockMargin{};

    QPushButton* exportButton;
    QPushButton* clearButton;

    QString lastDirPath;
    QString lastFileName;
    QString selectedNameFilter;

    void setButtonsEnabled(bool enabled);
    void appendToTextEdit(const char* timestamp, const char* text);
};


}
