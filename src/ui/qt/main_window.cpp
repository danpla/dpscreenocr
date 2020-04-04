
#include "main_window.h"

#include <cstdio>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
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
#include "action_chooser.h"
#include "history.h"
#include "hotkey_editor.h"
#include "lang_browser.h"
#include "status_indicator.h"
#include "utils.h"


namespace {


enum HotkeyAction {
    hotkeyActionToggleSelection,
    hotkeyActionCancelSelection
};


}


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
    }

    dpsoSetHotheysEnabled(true);
    dpsoCfgLoad(appFileName, cfgFileName);

    tabs = new QTabWidget();
    tabs->addTab(
        createMainTab(), pgettext("ui.tab", "Main"));
    tabs->addTab(createActionsTab(), _("Actions"));
    tabs->addTab(createHistoryTab(), _("History"));
    tabs->addTab(createAboutTab(), _("About"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);

    loadState();

    updateTimerId = startTimer(1000 / 60);
}


MainWindow::~MainWindow()
{
    dpsoShutdown();
}


MainWindow::DynamicStrings::DynamicStrings()
{
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


// The purpose of dummy tab widgets in create*Tab() methods is their
// layouts that will add margins around wrapped widgets.


QWidget* MainWindow::createActionsTab()
{
    actionChooser = new ActionChooser();
    connect(
        actionChooser, SIGNAL(actionsChanged()),
        this, SLOT(invalidateStatus()));

    auto* tabLayout = new QVBoxLayout();
    tabLayout->addWidget(actionChooser);
    tabLayout->addStretch();

    actionsTab = new QWidget();
    actionsTab->setLayout(tabLayout);
    return actionsTab;
}


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

    copyToClipboardTextSeparator = dpsoCfgGetStr(
        cfgKeyActionCopyToClipboardTextSeparator, "\n\n");
    runExeWaitToComplete = dpsoCfgGetBool(
        cfgKeyActionRunExecutableWaitToComplete, true);

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
    actionChooser->loadState();
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
    dpsoCfgSetBool(
        cfgKeyActionRunExecutableWaitToComplete,
        runExeWaitToComplete);

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
    actionChooser->saveState();
    langBrowser->saveState();

    initReadOnlyCfgKeys();

    dpsoCfgSetBool(
        cfgKeyOcrSplitTextBlocks,
        splitTextBlocksCheck->isChecked());

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
        && actionChooser->getSelectedActions());
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
    else if (!actionChooser->getSelectedActions())
        setStatus(Status::warning, dynStr.selectActions);
    else
        setStatus(Status::ok, dynStr.ready);

    statusValid = true;
}


void MainWindow::checkResult()
{
    if (!dpsoFetchResults(dpsoFetchFullChain))
        return;

    const DpsoJobResult* results;
    int numResults;
    dpsoGetFetchedResults(&results, &numResults);

    const auto actions = actionChooser->getSelectedActions();

    if (actions & ActionChooser::Action::copyToClipboard) {
        static QString fullText;
        fullText.clear();

        for (int i = 0; i < numResults; ++i) {
            if (!fullText.isEmpty())
                fullText += copyToClipboardTextSeparator;

            fullText += results[i].text;
        }

        auto* clipboard = QApplication::clipboard();
        clipboard->setText(fullText, QClipboard::Clipboard);
        clipboard->setText(fullText, QClipboard::Selection);
    }

    if (actions & ActionChooser::Action::addToHistory)
        for (int i = 0; i < numResults; ++i)
            history->append(results[i].text, results[i].timestamp);

    if (actions & ActionChooser::Action::runExe)
        for (int i = 0; i < numResults; ++i)
            dpsoExec(
                actionChooser->getExePath().toUtf8().data(),
                results[i].text,
                runExeWaitToComplete);
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
            DpsoJobArgs jobArgs;

            dpsoGetSelectionGeometry(&jobArgs.screenRect);

            jobArgs.flags = 0;
            if (splitTextBlocksCheck->isChecked())
                jobArgs.flags |= dpsoJobTextSegmentation;
            if (dpsoCfgGetBool(cfgKeyOcrDumpDebugImage, false))
                jobArgs.flags |= dpsoJobDumpDebugImage;

            dpsoQueueJob(&jobArgs);
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
