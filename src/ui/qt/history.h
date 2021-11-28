
#pragma once

#include <string>

#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QWidget>


class QCheckBox;
class QPushButton;
class QTextEdit;


class History : public QWidget {
    Q_OBJECT

public:
    History(const std::string& cfgDirPath, QWidget* parent = nullptr);

    void append(const char* text, const char* timestamp);

    bool loadState();
    void saveState() const;
private slots:
    void setWordWrap(bool wordWrap);
    void clear();
    void saveAs();
private:
    struct DynamicStrings {
        QString clearQuestion;
        QString cancel;
        QString clear;

        QString saveHistory;
        QString nameFilters;

        DynamicStrings();
    } dynStr;

    std::string historyFilePath;

    QCheckBox* wordWrapCheck;

    QTextEdit* textEdit;
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    int blockMargin;

    QPushButton* clearButton;
    QPushButton* saveAsButton;

    QString lastFileName;
    QString selectedNameFilter;

    void setButtonsEnabled(bool enabled);
    void appendToTextEdit(const char* text, const char* timestamp);
};
