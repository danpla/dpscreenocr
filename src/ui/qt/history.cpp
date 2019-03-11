
#include "history.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "dpso_utils/dpso_utils.h"
#include "dpso_utils/intl.h"
#define _(S) gettext(S)

#include "common/common.h"

#include "utils.h"


History::History(QWidget* parent)
     : QWidget(parent)
{
    wordWrapCheck = new QCheckBox(_("Wrap words"));
    wordWrapCheck->setChecked(true);
    connect(
        wordWrapCheck, SIGNAL(toggled(bool)),
        this, SLOT(setWordWrap(bool)));

    textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setWordWrapMode(QTextOption::WordWrap);

    charFormat.setFontPointSize(11);
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

    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    layout->addWidget(wordWrapCheck);
    layout->addWidget(textEdit);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(clearButton);
    buttonsLayout->addWidget(saveAsButton);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);
}


History::DynamicStrings::DynamicStrings()
{
    clearQuestion = _("Clear the history?");
    cancel = _("Cancel");
    clear = _("Clear");

    saveHistory = _("Save history");
    plainText = _("Plain text");
    allFiles = _("All files");
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
    QFileDialog dialog(this);
    dialog.setWindowTitle(dynStr.saveHistory);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(
        dynStr.plainText + " (*.txt);;"
        + "HTML (*.html *.htm);;"
        + "JSON (*.json);;"
        + dynStr.allFiles + " (*)");

    // selectNameFilter() is not guaranteed to work with native
    // dialogs, or maybe this is a bug with Qt 5.2.1 on my GTK
    // desktop.
    static QString selectedNameFilter;
    if (!selectedNameFilter.isEmpty())
        dialog.selectNameFilter(selectedNameFilter);

    if (!dpsoCfgGetBool(cfgKeyUiNativeFileDialogs, true))
        dialog.setOption(QFileDialog::DontUseNativeDialog);

    dialog.setViewMode(QFileDialog::Detail);

    const QDir dir(dpsoCfgGetStr(cfgKeyHistoryExportDir, ""));
    if (dir.exists())
        dialog.setDirectory(dir);

    if (!dialog.exec())
        return;

    const auto selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return;

    selectedNameFilter = dialog.selectedNameFilter();

    const auto lastDir = QDir::toNativeSeparators(
        dialog.directory().absolutePath());
    dpsoCfgSetStr(cfgKeyHistoryExportDir, lastDir.toUtf8().data());

    const auto fileName = QDir::toNativeSeparators(selectedFiles[0]);
    dpsoHistoryExportAuto(fileName.toUtf8().data());
}


void History::append(const char* text, const char* timestamp)
{
    const DpsoHistoryEntry entry {text, timestamp};
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
        dpsoCfgGetBool(cfgKeyHistoryWrapWords, true));

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

    dpsoCfgSetBool(
        cfgKeyHistoryWrapWords,
        wordWrapCheck->isChecked());
}
