
#include "main_window.h"

#include <cstdio>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QTabWidget>
#include <QTimerEvent>
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

#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "common/common.h"

#include "about.h"
#include "action_chooser.h"
#include "history.h"
#include "hotkey_editor.h"
#include "lang_browser.h"
#include "status_indicator.h"
#include "utils.h"


#define _(S) gettext(S)


namespace {


enum HotkeyAction {
    hotkeyActionToggleSelection,
    hotkeyActionCancelSelection
};


}


MainWindow::MainWindow()
    : QWidget{}
    , lastProgress{}
    , wasActiveLangs{}
    , statusValid{}
    , cancelSelectionHotkey{}
    , clipboardTextPending{}
    , minimizeToTray{}
    , minimizeOnStart{}
{
    setWindowTitle(appName);
    QApplication::setWindowIcon(getIcon(appFileName));

    if (!dpsoInit()) {
        QMessageBox::critical(nullptr, appName, dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    ocr.reset(dpsoOcrCreate());
    if (!ocr) {
        QMessageBox::critical(
            nullptr,
            appName,
            QString("Can't create OCR: ") + dpsoGetError());
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    const auto* cfgPath = dpsoGetCfgPath(appFileName);
    if (!cfgPath) {
        QMessageBox::critical(
            nullptr,
            appName,
            QString("Can't get configuration path: ")
                + dpsoGetError());

        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    cfgDirPath = cfgPath;
    cfgFilePath = cfgDirPath + *dpsoDirSeparators + cfgFileName;

    cfg.reset(dpsoCfgCreate());
    if (!cfg) {
        QMessageBox::critical(
            nullptr,
            appName,
            QString("Can't create Cfg: ") + dpsoGetError());
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    if (!dpsoCfgLoad(cfg.get(), cfgFilePath.c_str())) {
        QMessageBox::critical(
            nullptr,
            appName,
            QString("Can't load \"%1\": %2").arg(
                cfgFilePath.c_str(), dpsoGetError()));
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    dpsoSetHotheysEnabled(true);

    createQActions();

    tabs = new QTabWidget();
    tabs->addTab(
        createMainTab(), pgettext("ui.tab", "Main"));
    tabs->addTab(createActionsTab(), _("Actions"));
    tabs->addTab(createHistoryTab(), _("History"));
    tabs->addTab(createAboutTab(), _("About"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);

    createTrayIcon();

    if (!loadState(cfg.get())) {
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    if (minimizeOnStart) {
        if (trayIcon->isVisible() && minimizeToTray)
            visibilityAction->setChecked(false);
        else
            showMinimized();
    } else
        show();

    updateTimerId = startTimer(1000 / 60);
}


MainWindow::~MainWindow()
{
    dpsoShutdown();
}


MainWindow::DynamicStrings::DynamicStrings()
    : progress{_(
        "Recognition {progress}% ({current_job}/{total_jobs})")}
    , installLangs{_("Please install languages")}
    , selectLangs{_("Please select languages")}
    , selectActions{_(
        "Please select actions in the "
        "\342\200\234Actions\342\200\235 tab")}
    // Translators: Program is ready for OCR
    , ready{pgettext("ocr.status", "Ready")}
    , confirmQuitText{_(
        "Recognition is not yet finished. Quit anyway?")}
    , cancel{_("Cancel")}
    , quit{_("Quit")}
{

}


void MainWindow::timerEvent(QTimerEvent* event)
{
    (void)event;
    updateDpso();
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (dpsoOcrGetJobsPending(ocr.get())
            && !confirmation(
                this,
                dynStr.confirmQuitText, dynStr.cancel, dynStr.quit)) {
        event->ignore();
        return;
    }

    killTimer(updateTimerId);

    saveState(cfg.get());

    // On some platforms and desktop environments (like KDE), the app
    // keeps working in the background if the tray icon is not hidden
    // before quitting.
    trayIcon->hide();

    if (!dpsoCfgSave(cfg.get(), cfgFilePath.c_str()))
        QMessageBox::critical(
            this,
            appName,
            QString("Can't save \"%1\": %2").arg(
                cfgFilePath.c_str(), dpsoGetError()));

    dpsoSetHotheysEnabled(false);

    event->accept();
    qApp->quit();
}


void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange
            && isMinimized()
            && trayIcon->isVisible()
            && minimizeToTray)
        visibilityAction->setChecked(false);

    QWidget::changeEvent(event);
}


void MainWindow::invalidateStatus()
{
    statusValid = false;
}

void MainWindow::setVisibility(bool visible)
{
    if (!visible) {
        hide();
        return;
    }

    setWindowState(windowState() & ~Qt::WindowMinimized);

    show();
    raise();
    activateWindow();
}


void MainWindow::trayIconActivated(
    QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        visibilityAction->toggle();
}


void MainWindow::createQActions()
{
    visibilityAction = new QAction(
        dpsoStrNamedFormat(
            _("Show {app_name}"), {{"app_name", appName}}),
        this);
    visibilityAction->setCheckable(true);
    visibilityAction->setChecked(true);
    connect(
        visibilityAction, SIGNAL(toggled(bool)),
        this, SLOT(setVisibility(bool)));

    quitAction = new QAction(_("Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
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
    langBrowser = new LangBrowser(ocr.get());
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
    history = new History(cfgDirPath);

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


void MainWindow::createTrayIcon()
{
    auto* menu = new QMenu(this);
    menu->addAction(visibilityAction);
    menu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(getIcon(appFileName), this);
    trayIcon->setContextMenu(menu);

    connect(
        trayIcon,
        SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this,
        SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
}


bool MainWindow::loadState(const DpsoCfg* cfg)
{
    ocrAllowQueuing = dpsoCfgGetBool(
        cfg, cfgKeyOcrAllowQueuing, cfgDefaultValueOcrAllowQueuing);

    splitTextBlocksCheck->setChecked(
        dpsoCfgGetBool(
            cfg,
            cfgKeyOcrSplitTextBlocks,
            cfgDefaultValueOcrSplitTextBlocks));

    DpsoHotkey toggleSelectionHotkey;
    dpsoCfgGetHotkey(
        cfg,
        cfgKeyHotkeyToggleSelection,
        &toggleSelectionHotkey,
        &cfgDefaultValueHotkeyToggleSelection);
    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);

    hotkeyEditor->assignHotkey();

    dpsoCfgGetHotkey(
        cfg,
        cfgKeyHotkeyCancelSelection,
        &cancelSelectionHotkey,
        &cfgDefaultValueHotkeyCancelSelection);

    copyToClipboardTextSeparator = dpsoCfgGetStr(
        cfg,
        cfgKeyActionCopyToClipboardTextSeparator,
        cfgDefaultValueActionCopyToClipboardTextSeparator);
    runExeWaitToComplete = dpsoCfgGetBool(
        cfg,
        cfgKeyActionRunExecutableWaitToComplete,
        cfgDefaultValueActionRunExecutableWaitToComplete);

    tabs->setCurrentIndex(dpsoCfgGetInt(cfg, cfgKeyUiActiveTab, 0));

    const auto windowWidth = dpsoCfgGetInt(
        cfg, cfgKeyUiWindowWidth, 0);
    const auto windowHeight = dpsoCfgGetInt(
        cfg, cfgKeyUiWindowHeight, 0);

    if (windowWidth > 0 && windowHeight > 0) {
        move(
            dpsoCfgGetInt(cfg, cfgKeyUiWindowX, x()),
            dpsoCfgGetInt(cfg, cfgKeyUiWindowY, y()));
        resize(windowWidth, windowHeight);
    }

    if (dpsoCfgGetBool(cfg, cfgKeyUiWindowMaximized, false))
        setWindowState(windowState() | Qt::WindowMaximized);

    langBrowser->loadState(cfg);
    actionChooser->loadState(cfg);
    if (!history->loadState(cfg))
        return false;

    trayIcon->setVisible(
        dpsoCfgGetBool(
            cfg,
            cfgKeyUiTrayIconVisible,
            cfgDefaultValueUiTrayIconVisible));
    minimizeToTray = dpsoCfgGetBool(
        cfg,
        cfgKeyUiWindowMinimizeToTray,
        cfgDefaultValueUiWindowMinimizeToTray);

    minimizeOnStart = dpsoCfgGetBool(
        cfg,
        cfgKeyUiWindowMinimizeOnStart,
        cfgDefaultValueUiWindowMinimizeOnStart);

    return true;
}


void MainWindow::saveState(DpsoCfg* cfg) const
{
    dpsoCfgSetBool(cfg, cfgKeyOcrAllowQueuing, ocrAllowQueuing);

    dpsoCfgSetBool(
        cfg,
        cfgKeyOcrSplitTextBlocks,
        splitTextBlocksCheck->isChecked());

    DpsoHotkey toggleSelectionHotkey;
    dpsoFindActionHotkey(
        hotkeyActionToggleSelection, &toggleSelectionHotkey);
    dpsoCfgSetHotkey(
        cfg, cfgKeyHotkeyToggleSelection, &toggleSelectionHotkey);

    dpsoCfgSetHotkey(
        cfg, cfgKeyHotkeyCancelSelection, &cancelSelectionHotkey);

    dpsoCfgSetStr(
        cfg,
        cfgKeyActionCopyToClipboardTextSeparator,
        copyToClipboardTextSeparator.toUtf8().data());
    dpsoCfgSetBool(
        cfg,
        cfgKeyActionRunExecutableWaitToComplete,
        runExeWaitToComplete);

    if (!dpsoCfgKeyExists(cfg, cfgKeyUiNativeFileDialogs))
        dpsoCfgSetBool(
            cfg,
            cfgKeyUiNativeFileDialogs,
            cfgDefaultValueUiNativeFileDialogs);

    dpsoCfgSetInt(cfg, cfgKeyUiActiveTab, tabs->currentIndex());

    if (!isMaximized()) {
        dpsoCfgSetInt(cfg, cfgKeyUiWindowX, x());
        dpsoCfgSetInt(cfg, cfgKeyUiWindowY, y());
        dpsoCfgSetInt(cfg, cfgKeyUiWindowWidth, width());
        dpsoCfgSetInt(cfg, cfgKeyUiWindowHeight, height());
    }

    dpsoCfgSetBool(cfg, cfgKeyUiWindowMaximized, isMaximized());

    langBrowser->saveState(cfg);
    actionChooser->saveState(cfg);
    history->saveState(cfg);

    dpsoCfgSetBool(
        cfg, cfgKeyUiTrayIconVisible, trayIcon->isVisible());
    dpsoCfgSetBool(cfg, cfgKeyUiWindowMinimizeToTray, minimizeToTray);

    dpsoCfgSetBool(
        cfg, cfgKeyUiWindowMinimizeOnStart, minimizeOnStart);
}


bool MainWindow::canStartSelection() const
{
    return (
        dpsoOcrGetNumActiveLangs(ocr.get()) > 0
        && (ocrAllowQueuing || !dpsoOcrGetJobsPending(ocr.get()))
        && actionChooser->getSelectedActions());
}


void MainWindow::updateDpso()
{
    dpsoUpdate();

    updateStatus();
    checkResults();
    checkHotkeyActions();
}


void MainWindow::setStatus(Status newStatus, const QString& text)
{
    const auto textWithAppName = text + " - " + appName;

    setWindowTitle(
        newStatus == Status::ok ? appName : textWithAppName);

    #if DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    XFlush(QX11Info::display());
    #endif

    statusIndicator->setStatus(newStatus);
    statusLabel->setText(text);

    trayIcon->setToolTip(textWithAppName);
}


void MainWindow::updateStatus()
{
    DpsoOcrProgress progress;
    dpsoOcrGetProgress(ocr.get(), &progress);

    actionsTab->setEnabled(progress.totalJobs == 0);

    if (progress.totalJobs > 0) {
        // Update status when the progress ends.
        statusValid = false;

        if (dpsoOcrProgressEqual(&progress, &lastProgress))
            return;

        lastProgress = progress;

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
                dynStr.progress.c_str(),
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
    const auto hasActiveLangs = (
        dpsoOcrGetNumActiveLangs(ocr.get()) > 0);
    if (hasActiveLangs != wasActiveLangs) {
        wasActiveLangs = hasActiveLangs;
        statusValid = false;
    }

    if (statusValid)
        return;
    if (dpsoOcrGetNumLangs(ocr.get()) == 0)
        setStatus(Status::warning, dynStr.installLangs);
    else if (dpsoOcrGetNumActiveLangs(ocr.get()) == 0)
        setStatus(Status::warning, dynStr.selectLangs);
    else if (!actionChooser->getSelectedActions())
        setStatus(Status::warning, dynStr.selectActions);
    else
        setStatus(Status::ok, dynStr.ready);

    statusValid = true;
}


void MainWindow::checkResults()
{
    // Check if jobs are completed before fetching results, since new
    // jobs may finish right after dpsoFetchResults() and before
    // dpsoGetJobsPending() call.
    const auto jobsCompleted = !dpsoOcrGetJobsPending(ocr.get());

    DpsoOcrJobResults results;
    dpsoOcrFetchResults(ocr.get(), &results);

    const auto actions = actionChooser->getSelectedActions();

    QByteArray exePath;
    if ((actions & ActionChooser::Action::runExe)
            && results.numItems > 0)
        exePath = actionChooser->getExePath().toUtf8();

    for (int i = 0; i < results.numItems; ++i) {
        const auto& result = results.items[i];

        if (actions & ActionChooser::Action::copyToClipboard) {
            // We need the clipboardTextPending flag since the result
            // text may be empty, yet it should still be copied to the
            // clipboard and separated from other texts.
            if (clipboardTextPending)
                clipboardText += copyToClipboardTextSeparator;

            clipboardText += QString::fromUtf8(
                result.text, result.textLen);
            clipboardTextPending = true;
        }

        if (actions & ActionChooser::Action::addToHistory)
            history->append(result.timestamp, result.text);

        if (actions & ActionChooser::Action::runExe)
            dpsoExec(
                exePath.data(), result.text, runExeWaitToComplete);
    }

    if (jobsCompleted && clipboardTextPending) {
        QApplication::clipboard()->setText(clipboardText);
        clipboardText.clear();
        clipboardTextPending = false;
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
            DpsoOcrJobArgs jobArgs;

            dpsoGetSelectionGeometry(&jobArgs.screenRect);
            if (dpsoRectIsEmpty(&jobArgs.screenRect))
                return;

            jobArgs.flags = 0;
            if (splitTextBlocksCheck->isChecked())
                jobArgs.flags |= dpsoOcrJobTextSegmentation;

            if (!dpsoOcrQueueJob(ocr.get(), &jobArgs))
                QMessageBox::warning(
                    this,
                    appName,
                    QString("Can't queue OCR job: ")
                        + dpsoGetError());
        }
    } else if (hotkeyAction == hotkeyActionToggleSelection
            && canStartSelection()) {
        dpsoSetSelectionIsEnabled(true);
        dpsoBindHotkey(
            &cancelSelectionHotkey, hotkeyActionCancelSelection);
    }
}
