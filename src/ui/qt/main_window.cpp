
#include "main_window.h"

#include <cstdio>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "default_config.h"

// Workaround for QTBUG-33775
#if DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
#include <QX11Info>
#include <X11/Xlib.h>
// We only need XFlush().
// Undef X11 macros that clash with Qt and our names.
#undef Bool
#undef None
#undef Status
#endif

#include "dpso/dpso.h"
#include "dpso_utils/dpso_utils.h"
#include "dpso_utils/intl.h"
#define _(S) gettext(S)

#include "common/common.h"

#include "about.h"
#include "history.h"
#include "hotkey_editor.h"
#include "lang_browser.h"
#include "status_indicator.h"
#include "utils.h"


const DpsoHotkey cancelSelectionDefaultHotkey {
    dpsoKeyEscape, dpsoKeyModNone};


MainWindow::MainWindow()
    : QWidget()
    , wasActiveLangs {}
    , statusValid {}
{
    setWindowTitle(appName);
    QApplication::setWindowIcon(getIcon(appFileName));

    if (!dpsoInit()) {
        QMessageBox::critical(
            nullptr, QString(appName) + " error", dpsoGetError());
        std::exit(EXIT_FAILURE);
        return;
    }

    dpsoSetHotheysEnabled(true);
    dpsoCfgLoad(appFileName, cfgFileName);

    tabs = new QTabWidget();
    tabs->addTab(
        createMainTab(), pgettext("ui.tab", "Main"));
    tabs->addTab(createActionsTab(), _("Actions"));
    tabs->addTab(createHistoryTab(), _("History"));
    tabs->addTab(createAboutTab(), _("About"));

    auto* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(tabs);
    setLayout(mainLayout);

    loadState();

    updateTimerId = startTimer(1000 / 60);
}


MainWindow::~MainWindow()
{
    dpsoShutdown();
}


MainWindow::DynamicStrings::DynamicStrings()
{
    chooseExeDialogTitle = _("Choose an executable");

    progress = _(
        "Recognition {progress}% ({current_job}/{total_jobs})");

    installLangs = _("Please install languages");
    selectLangs = _("Please select languages");
    selectActions = _(
        "Please select actions in the "
        "\342\200\234Actions\342\200\235 tab");
    // Translators: Program is ready for OCR
    ready = pgettext("ocr.status", "Ready");

    confirmQuitText = _(
        "Recognition is not yet finished. Quit anyway?");
    cancel = _("Cancel");
    quit = _("Quit");
}


void MainWindow::timerEvent(QTimerEvent* event)
{
    (void)event;
    updateDpso();
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (dpsoGetJobsPending()
            && !confirmation(
                this,
                dynStr.confirmQuitText, dynStr.cancel, dynStr.quit)) {
        event->ignore();
        return;
    }

    killTimer(updateTimerId);

    saveState();

    dpsoCfgSave(appFileName, cfgFileName);
    dpsoSetHotheysEnabled(false);

    event->accept();
}


void MainWindow::chooseExe()
{
    QFileDialog::Options options = 0;
    if (!dpsoCfgGetBool(cfgKeyUiNativeFileDialogs, true))
        options |= QFileDialog::DontUseNativeDialog;

    const auto fileName = QFileDialog::getOpenFileName(
        this, dynStr.chooseExeDialogTitle, "", "", nullptr, options);
    if (fileName.isEmpty())
        return;

    exeLineEdit->setText(QDir::toNativeSeparators(fileName));
}


void MainWindow::invalidateStatus()
{
    statusValid = false;
}


QWidget* MainWindow::createMainTab()
{
    auto* statusGroup = new QGroupBox(_("Status"));
    auto* statusGroupLayout = new QHBoxLayout();
    statusGroup->setLayout(statusGroupLayout);

    statusIndicator = new StatusIndicator();
    statusGroupLayout->addWidget(statusIndicator);

    statusLabel = new QLabel();
    statusLabel->setTextFormat(Qt::PlainText);
    statusGroupLayout->addWidget(statusLabel, 1);

    auto* ocrGroup = new QGroupBox(_("Character recognition"));
    auto* ocrGroupLayout = new QVBoxLayout();
    ocrGroup->setLayout(ocrGroupLayout);

    splitTextBlocksCheck = new QCheckBox(
        _("Split text blocks"));
    splitTextBlocksCheck->setToolTip(
        _("Split independent text blocks, e.g. columns"));
    ocrGroupLayout->addWidget(splitTextBlocksCheck);

    ocrGroupLayout->addWidget(new QLabel(_("Languages:")));
    langBrowser = new LangBrowser();
    ocrGroupLayout->addWidget(langBrowser);

    auto* hotkeyGroup = new QGroupBox(_("Hotkey"));
    auto* hotkeyGroupLayout = new QVBoxLayout();
    hotkeyGroup->setLayout(hotkeyGroupLayout);

    hotkeyEditor = new HotkeyEditor(
        hotkeyActionToggleSelection,
        {dpsoUnknownKey, dpsoKeyEscape},
        Qt::Vertical);
    connect(
        hotkeyEditor, SIGNAL(changed()),
        hotkeyEditor, SLOT(bind()));
    hotkeyGroupLayout->addWidget(hotkeyEditor);

    auto* tabLayout = new QVBoxLayout();
    tabLayout->addWidget(statusGroup);
    tabLayout->addWidget(ocrGroup);
    tabLayout->addWidget(hotkeyGroup);

    auto* tab = new QWidget();
    tab->setLayout(tabLayout);
    return tab;
}


QWidget* MainWindow::createActionsTab()
{
    copyToClipboardCheck = new QCheckBox(_("Copy text to clipboard"));
    addToHistoryCheck = new QCheckBox(_("Add text to history"));

    runExeCheck = new QCheckBox(_("Run executable:"));
    runExeCheck->setToolTip(
        _("Run an executable with the recognized text as the "
            "first argument"));
    exeLineEdit = new QLineEdit();
    exeLineEdit->setEnabled(false);

    auto* selectExeButton = new QPushButton("\342\200\246");
    selectExeButton->setEnabled(false);
    connect(
        selectExeButton, SIGNAL(clicked()),
        this, SLOT(chooseExe()));

    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        exeLineEdit, SLOT(setEnabled(bool)));
    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        selectExeButton, SLOT(setEnabled(bool)));

    connect(
        copyToClipboardCheck, SIGNAL(toggled(bool)),
        this, SLOT(invalidateStatus()));
    connect(
        addToHistoryCheck, SIGNAL(toggled(bool)),
        this, SLOT(invalidateStatus()));
    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        this, SLOT(invalidateStatus()));

    auto* tabLayout = new QVBoxLayout();
    tabLayout->addWidget(copyToClipboardCheck);
    tabLayout->addWidget(addToHistoryCheck);

    auto* execLayout = new QHBoxLayout();
    execLayout->addWidget(runExeCheck);
    execLayout->addWidget(exeLineEdit, 1);
    execLayout->addWidget(selectExeButton);
    tabLayout->addLayout(execLayout);

    tabLayout->addStretch();

    actionsTab = new QWidget();
    actionsTab->setLayout(tabLayout);
    return actionsTab;
}


// The purpose of dummy widgets in createHistoryTab() and
// createAboutTab() is their layouts that will add margins around
// History and About widgets.


QWidget* MainWindow::createHistoryTab()
{
    history = new History();

    auto* tabLayout = new QVBoxLayout();
    tabLayout->addWidget(history);

    auto* tab = new QWidget();
    tab->setLayout(tabLayout);
    return tab;
}


QWidget* MainWindow::createAboutTab()
{
    auto* about = new About();

    auto* tabLayout = new QVBoxLayout();
    tabLayout->addWidget(about);

    auto* tab = new QWidget();
    tab->setLayout(tabLayout);
    return tab;
}


void MainWindow::loadState()
{
    ocrAllowQueuing = dpsoCfgGetBool(cfgKeyOcrAllowQueuing, true);

    splitTextBlocksCheck->setChecked(
        dpsoCfgGetBool(cfgKeyOcrSplitTextBlocks, false));

    const DpsoHotkey toggleSelectionDefaultHotkey {
        dpsoKeyGrave, dpsoKeyModCtrl};
    DpsoHotkey toggleSelectionHotkey;
    dpsoCfgGetHotkey(
        cfgKeyHotkeyToggleSelection,
        &toggleSelectionHotkey,
        &toggleSelectionDefaultHotkey);
    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);

    hotkeyEditor->assignHotkey();

    copyToClipboardCheck->setChecked(
        dpsoCfgGetBool(cfgKeyActionCopyToClipboard, false));
    addToHistoryCheck->setChecked(
        dpsoCfgGetBool(cfgKeyActionAddToHistory, true));
    runExeCheck->setChecked(
        dpsoCfgGetBool(cfgKeyActionRunExecutable, false));
    exeLineEdit->setText(
        dpsoCfgGetStr(cfgKeyActionRunExecutablePath, ""));

    copyToClipboardTextSeparator = dpsoCfgGetStr(
        cfgKeyActionCopyToClipboardTextSeparator, "\n\n");
    runExeTextSeparator = dpsoCfgGetStr(
        cfgKeyActionRunExecutableTextSeparator, "\n\n");
    sameTextSeparators = (
        copyToClipboardTextSeparator == runExeTextSeparator);

    tabs->setCurrentIndex(dpsoCfgGetInt(cfgKeyUiActiveTab, 0));

    const auto windowWidth = dpsoCfgGetInt(cfgKeyUiWindowWidth, 0);
    const auto windowHeight = dpsoCfgGetInt(cfgKeyUiWindowHeight, 0);

    if (windowWidth > 0 && windowHeight > 0) {
        move(
            dpsoCfgGetInt(cfgKeyUiWindowX, x()),
            dpsoCfgGetInt(cfgKeyUiWindowY, y()));
        resize(windowWidth, windowHeight);
    }

    if (dpsoCfgGetBool(cfgKeyUiWindowMaximized, false))
        setWindowState(windowState() | Qt::WindowMaximized);

    langBrowser->loadState();
    history->loadState();
}


void MainWindow::initReadOnlyCfgKeys() const
{
    // The following options are only available in CFG and can't be
    // changed via GUI. The purpose of cfgSet*() calls is to add the
    // fields in CFG if they don't already exist.

    dpsoCfgSetStr(
        cfgKeyActionCopyToClipboardTextSeparator,
        copyToClipboardTextSeparator.toUtf8().data());
    dpsoCfgSetStr(
        cfgKeyActionRunExecutableTextSeparator,
        runExeTextSeparator.toUtf8().data());

    dpsoCfgSetBool(cfgKeyOcrAllowQueuing, ocrAllowQueuing);
    dpsoCfgSetBool(
        cfgKeyOcrDumpDebugImage,
        dpsoCfgGetBool(cfgKeyOcrDumpDebugImage, false));

    dpsoCfgSetBool(
        cfgKeyUiNativeFileDialogs,
        dpsoCfgGetBool(cfgKeyUiNativeFileDialogs, true));

    DpsoHotkey cancelSelectionHotkey;
    dpsoCfgGetHotkey(
        cfgKeyHotkeyCancelSelection,
        &cancelSelectionHotkey,
        &cancelSelectionDefaultHotkey);
    dpsoCfgSetHotkey(
        cfgKeyHotkeyCancelSelection, &cancelSelectionHotkey);
}


void MainWindow::saveState() const
{
    history->saveState();
    langBrowser->saveState();

    initReadOnlyCfgKeys();

    dpsoCfgSetBool(
        cfgKeyOcrSplitTextBlocks,
        splitTextBlocksCheck->isChecked());

    dpsoCfgSetBool(
        cfgKeyActionCopyToClipboard,
        copyToClipboardCheck->isChecked());
    dpsoCfgSetBool(
        cfgKeyActionAddToHistory,
        addToHistoryCheck->isChecked());
    dpsoCfgSetBool(
        cfgKeyActionRunExecutable,
        runExeCheck->isChecked());
    dpsoCfgSetStr(
        cfgKeyActionRunExecutablePath,
        exeLineEdit->text().toUtf8().data());

    dpsoCfgSetInt(cfgKeyUiActiveTab, tabs->currentIndex());

    dpsoCfgSetBool(cfgKeyUiWindowMaximized, isMaximized());
    if (!isMaximized()) {
        dpsoCfgSetInt(cfgKeyUiWindowX, x());
        dpsoCfgSetInt(cfgKeyUiWindowY, y());
        dpsoCfgSetInt(cfgKeyUiWindowWidth, width());
        dpsoCfgSetInt(cfgKeyUiWindowHeight, height());
    }

    DpsoHotkey toggleSelectionHotkey;
    dpsoFindActionHotkey(
        hotkeyActionToggleSelection, &toggleSelectionHotkey);
    dpsoCfgSetHotkey(
        cfgKeyHotkeyToggleSelection, &toggleSelectionHotkey);
}


bool MainWindow::canStartSelection() const
{
    return (
        dpsoGetNumActiveLangs() > 0
        && (ocrAllowQueuing || !dpsoGetJobsPending())
        && (copyToClipboardCheck->isChecked()
            || addToHistoryCheck->isChecked()
            || runExeCheck->isChecked()));
}


void MainWindow::updateDpso()
{
    dpsoUpdate();

    updateStatus();
    checkResult();
    checkHotkeyActions();
}


void MainWindow::setStatus(Status newStatus, const QString& text)
{
    if (newStatus == Status::ok)
        setWindowTitle(appName);
    else
        setWindowTitle(text + " - " + appName);

    #if DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    XFlush(QX11Info::display());
    #endif

    statusIndicator->setStatus(newStatus);
    statusLabel->setText(text);
}


void MainWindow::updateStatus()
{
    DpsoProgress progress;
    int progressIsNew;
    dpsoGetProgress(&progress, &progressIsNew);

    actionsTab->setEnabled(progress.totalJobs == 0);

    if (progress.totalJobs > 0) {
        // Update status when the progress ends.
        statusValid = false;

        if (!progressIsNew)
            return;

        int totalProgress;
        if (progress.curJob == 0)
            totalProgress = 0;
        else
            totalProgress = (
                ((progress.curJob - 1) * 100
                    + progress.curJobProgress)
                / progress.totalJobs);

        setStatus(
            Status::busy,
            dpsoStrNamedFormat(
                dynStr.progress.toUtf8().data(),
                {{"progress",
                        std::to_string(totalProgress).c_str()},
                    {"current_job",
                        std::to_string(
                            progress.curJob).c_str()},
                    {"total_jobs",
                        std::to_string(
                            progress.totalJobs).c_str()}}));

        return;
    }

    // Invalidate status when the number of active languages
    // changes from or to 0.
    const auto hasActiveLangs = dpsoGetNumActiveLangs() > 0;
    if (hasActiveLangs != wasActiveLangs) {
        wasActiveLangs = hasActiveLangs;
        statusValid = false;
    }

    if (statusValid)
        return;

    if (dpsoGetNumLangs() == 0)
        setStatus(Status::warning, dynStr.installLangs);
    else if (dpsoGetNumActiveLangs() == 0)
        setStatus(Status::warning, dynStr.selectLangs);
    else if (!copyToClipboardCheck->isChecked()
                && !addToHistoryCheck->isChecked()
                && !runExeCheck->isChecked())
        setStatus(Status::warning, dynStr.selectActions);
    else
        setStatus(Status::ok, dynStr.ready);

    statusValid = true;
}


static void concatResults(
    const DpsoJobResult* results,
    int numResults,
    const QString& separator,
    QString& str)
{
    str.clear();

    for (int i = 0; i < numResults; ++i) {
        if (!str.isEmpty())
            str += separator;

        str += results[i].text;
    }
}


void MainWindow::checkResult()
{
    if (!dpsoFetchResults(true))
        return;

    const DpsoJobResult* results;
    int numResults;
    dpsoGetFetchedResults(&results, &numResults);

    if (copyToClipboardCheck->isChecked()
            || runExeCheck->isChecked()) {
        static QString fullText;
        fullText.clear();

        if (copyToClipboardCheck->isChecked()) {
            concatResults(
                results, numResults,
                copyToClipboardTextSeparator, fullText);

            auto* clipboard = QApplication::clipboard();
            clipboard->setText(fullText, QClipboard::Clipboard);
            clipboard->setText(fullText, QClipboard::Selection);
        }

        if (runExeCheck->isChecked()) {
            if (fullText.isEmpty() || !sameTextSeparators)
                concatResults(
                    results, numResults,
                    runExeTextSeparator, fullText);

            dpsoExec(
                exeLineEdit->text().toUtf8().data(),
                fullText.toUtf8().data());
        }
    }

    if (addToHistoryCheck->isChecked()) {
        for (int i = 0; i < numResults; ++i)
            history->append(results[i].text, results[i].timestamp);
    }
}


void MainWindow::checkHotkeyActions()
{
    const auto hotkeyAction = dpsoGetLastHotkeyAction();

    if (dpsoGetSelectionIsEnabled()) {
        if (hotkeyAction != hotkeyActionToggleSelection
                && hotkeyAction != hotkeyActionCancelSelection)
            return;

        dpsoSetSelectionIsEnabled(false);
        dpsoUnbindAction(hotkeyActionCancelSelection);

        if (hotkeyAction == hotkeyActionToggleSelection) {
            int x, y, w, h;
            dpsoGetSelectionGeometry(&x, &y, &w, &h);

            int jobFlags = 0;
            if (splitTextBlocksCheck->isChecked())
                jobFlags |= dpsoJobTextSegmentation;
            if (dpsoCfgGetBool(cfgKeyOcrDumpDebugImage, false))
                jobFlags |= dpsoJobDumpDebugImage;

            dpsoQueueJob(x, y, w, h, jobFlags);
        }
    } else if (hotkeyAction == hotkeyActionToggleSelection
            && canStartSelection()) {
        dpsoSetSelectionIsEnabled(true);

        DpsoHotkey cancelSelectionHotkey;
        dpsoCfgGetHotkey(
            cfgKeyHotkeyCancelSelection,
            &cancelSelectionHotkey,
            &cancelSelectionDefaultHotkey);

        dpsoBindHotkey(
            &cancelSelectionHotkey, hotkeyActionCancelSelection);
    }
}
