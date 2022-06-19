
#pragma once

#include <string>

#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QWidget>

#include "dpso_utils/dpso_utils.h"


class QCheckBox;
class QPushButton;
class QTextEdit;


class History : public QWidget {
    Q_OBJECT

public:
    explicit History(
        const std::string& dirPath, QWidget* parent = nullptr);

    void append(const char* timestamp, const char* text);

    bool loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
private slots:
    void setWordWrap(bool wordWrap);
    void doExport();
    void clear();
private:
    struct DynamicStrings {
        QString exportHistory;
        QString nameFilters;

        QString clearQuestion;
        QString cancel;
        QString clear;

        DynamicStrings();
    } dynStr;

    std::string historyFilePath;
    dpso::HistoryUPtr history;

    QCheckBox* wordWrapCheck;

    QTextEdit* textEdit;
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    int blockMargin;

    QPushButton* exportButton;
    QPushButton* clearButton;

    bool nativeFileDialogs;
    QString lastDirPath;
    QString lastFileName;
    QString selectedNameFilter;

    void setButtonsEnabled(bool enabled);
    void appendToTextEdit(const char* text, const char* timestamp);
};
