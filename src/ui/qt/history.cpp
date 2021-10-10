
#include "history.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "common/common.h"

#include "utils.h"


#define _(S) gettext(S)


History::History(QWidget* parent)
    : QWidget{parent}
{
    wordWrapCheck = new QCheckBox(_("Wrap words"));
    wordWrapCheck->setChecked(true);
    connect(
        wordWrapCheck, SIGNAL(toggled(bool)),
        this, SLOT(setWordWrap(bool)));

    textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setWordWrapMode(QTextOption::WordWrap);

    blockFormat.setLineHeight(
        140, QTextBlockFormat::ProportionalHeight);
    blockMargin = QFontMetrics(charFormat.font()).height();

    clearButton = new QPushButton(_("Clear"));
    connect(
        clearButton, SIGNAL(clicked()),
        this, SLOT(clear()));

    saveAsButton = new QPushButton(_("Save as\342\200\246"));
    saveAsButton->setToolTip(
        _("Save the history as plain text, HTML, or JSON"));
    connect(
        saveAsButton, SIGNAL(clicked()),
        this, SLOT(saveAs()));

    setButtonsEnabled(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(wordWrapCheck);
    layout->addWidget(textEdit);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(clearButton);
    buttonsLayout->addWidget(saveAsButton);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);

    lastFileName = _("History");
}


History::DynamicStrings::DynamicStrings()
    : clearQuestion{_("Clear the history?")}
    , cancel{_("Cancel")}
    , clear{_("Clear")}
    , saveHistory{_("Save history")}
    , nameFilters{
        QString{_("Plain text")} + " (*.txt);;"
        + "HTML (*.html *.htm);;"
        + "JSON (*.json);;"
        + _("All files") + " (*)"}
{

}


void History::setButtonsEnabled(bool enabled)
{
    clearButton->setEnabled(enabled);
    saveAsButton->setEnabled(enabled);
}


void History::setWordWrap(bool wordWrap)
{
    textEdit->setWordWrapMode(
        wordWrap ? QTextOption::WordWrap : QTextOption::NoWrap);
}


void History::clear()
{
    if (!confirmation(
            this,
            dynStr.clearQuestion, dynStr.cancel, dynStr.clear))
        return;

    dpsoHistoryClear();
    textEdit->clear();
    setButtonsEnabled(false);
}


void History::saveAs()
{
    QFileDialog::Options options;
    if (!dpsoCfgGetBool(
            cfgKeyUiNativeFileDialogs,
            cfgDefaultValueUiNativeFileDialogs))
        options |= QFileDialog::DontUseNativeDialog;

    QString dirPath = dpsoCfgGetStr(cfgKeyHistoryExportDir, "");
    // Don't pass an empty path to QDir since in this case QDir points
    // to the current working directory.
    if (dirPath.isEmpty() || !QDir(dirPath).exists())
        dirPath = QDir::homePath();

    const auto filePath = QDir::toNativeSeparators(
        QFileDialog::getSaveFileName(
            this,
            dynStr.saveHistory,
            QDir(dirPath).filePath(lastFileName),
            dynStr.nameFilters,
            &selectedNameFilter,
            options));
    if (filePath.isEmpty())
        return;

    const auto filePathUtf8 = filePath.toUtf8();
    dpsoHistoryExport(
        filePathUtf8.data(),
        dpsoHistoryDetectExportFormat(
            filePathUtf8.data(), dpsoHistoryExportFormatPlainText));

    const QFileInfo fileInfo(filePath);

    lastFileName = fileInfo.fileName();

    dirPath = QDir::toNativeSeparators(fileInfo.dir().absolutePath());
    dpsoCfgSetStr(cfgKeyHistoryExportDir, dirPath.toUtf8().data());
}


void History::append(const char* text, const char* timestamp)
{
    const DpsoHistoryEntry entry{text, timestamp};
    dpsoHistoryAppend(&entry);

    appendToTextEdit(text, timestamp);

    setButtonsEnabled(true);
}


void History::appendToTextEdit(
    const char* text, const char* timestamp)
{
    auto cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);

    charFormat.setFontWeight(QFont::Bold);
    blockFormat.setLeftMargin(0);
    blockFormat.setRightMargin(0);
    cursor.insertBlock(blockFormat, charFormat);
    cursor.insertText(timestamp);

    // Remember the position so we can scroll up later.
    cursor.movePosition(QTextCursor::StartOfBlock);
    const auto textBegin = cursor.position();
    cursor.movePosition(QTextCursor::End);

    charFormat.setFontWeight(QFont::Normal);
    blockFormat.setLeftMargin(blockMargin);
    blockFormat.setRightMargin(blockMargin);
    cursor.insertBlock(blockFormat, charFormat);

    cursor.insertText(QString(text).trimmed());
    cursor.insertHtml("<hr>");

    // We must scroll to the bottom before scrolling to textBegin,
    // so that textBegin becomes the first line in the viewport
    // rather than the last.
    textEdit->setTextCursor(cursor);
    cursor.setPosition(textBegin);
    textEdit->setTextCursor(cursor);
}


void History::loadState()
{
    wordWrapCheck->setChecked(
        dpsoCfgGetBool(
            cfgKeyHistoryWrapWords,
            cfgDefaultValueHistoryWrapWords));

    dpsoHistoryLoad(appFileName, historyFileName);

    textEdit->clear();
    for (int i = 0; i < dpsoHistoryCount(); ++i) {
        DpsoHistoryEntry entry;
        dpsoHistoryGet(i, &entry);
        appendToTextEdit(entry.text, entry.timestamp);
    }

    setButtonsEnabled(dpsoHistoryCount() > 0);
}


void History::saveState() const
{
    dpsoHistorySave(appFileName, historyFileName);

    // Add a field to CFG in case the history has never been exported,
    // just to make sure that the user will see all available options
    // in the CFG file.
    if (!dpsoCfgKeyExists(cfgKeyHistoryExportDir))
        dpsoCfgSetStr(cfgKeyHistoryExportDir, "");

    dpsoCfgSetBool(
        cfgKeyHistoryWrapWords,
        wordWrapCheck->isChecked());
}
