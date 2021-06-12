
#pragma once

#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>


class History : public QWidget {
    Q_OBJECT

public:
    explicit History(QWidget* parent = nullptr);

    void append(const char* text, const char* timestamp);

    void loadState();
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
        QString plainText;
        QString allFiles;

        DynamicStrings();
    } dynStr;

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
