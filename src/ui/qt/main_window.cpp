
#include "main_window.h"

#include <cstdlib>

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
#include <QSessionManager>
#include <QTabWidget>
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QtGlobal>

#include "dpso_intl/dpso_intl.h"

#include "about.h"
#include "action_chooser.h"
#include "history.h"
#include "hotkey_editor.h"
#include "lang_browser.h"
#include "status_indicator.h"
#include "utils.h"


#define _(S) gettext(S)


enum : DpsoHotkeyAction {
    hotkeyActionToggleSelection,
    hotkeyActionCancelSelection
};


MainWindow::MainWindow()
    : QWidget{}
    , progressStatusFmt{_(
        "Recognition {progress}% ({current_job}/{total_jobs})")}
    , ocrAllowQueuing{}
    , lastProgress{}
    , wasActiveLangs{}
    , statusValid{}
    , lastStatus{}
    , cancelSelectionHotkey{}
    , clipboardTextPending{}
    , minimizeToTray{}
    , minimizeOnStart{}
    , selectionBorderWidth{}
{
    setWindowTitle(uiAppName);

    if (!dpsoInit()) {
        QMessageBox::critical(nullptr, uiAppName, dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    if (!uiStartupSetup()) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Startup setup failed: ") + dpsoGetError());
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    const auto* cfgPath = dpsoGetUserDir(
        DpsoUserDirConfig, uiAppFileName);
    if (!cfgPath) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Can't get configuration path: ")
                + dpsoGetError());

        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    cfgDirPath = cfgPath;
    cfgFilePath = cfgDirPath + *dpsoDirSeparators + uiCfgFileName;

    cfg.reset(dpsoCfgCreate());
    if (!cfg) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Can't create Cfg: ") + dpsoGetError());
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    if (!dpsoCfgLoad(cfg.get(), cfgFilePath.c_str())) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Can't load \"%1\": %2").arg(
                cfgFilePath.c_str(), dpsoGetError()));
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    const auto* dataPath = dpsoGetUserDir(
        DpsoUserDirData, uiAppFileName);
    if (!dataPath) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Can't get data path: ") + dpsoGetError());

        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    if (dpsoOcrGetNumEngines() == 0) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("No OCR engines are available"));

        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    // Our GUI currently doesn't allow to select an engine.
    DpsoOcrEngineInfo ocrEngineInfo;
    dpsoOcrGetEngineInfo(0, &ocrEngineInfo);

    std::string ocrDataDirPath;
    const char* dataDirName = uiGetOcrDataDirName(&ocrEngineInfo);
    if (*dataDirName)
        ocrDataDirPath =
            std::string{dataPath} + *dpsoDirSeparators + dataDirName;

    const DpsoOcrArgs ocrArgs{0, ocrDataDirPath.c_str()};
    ocr.reset(dpsoOcrCreate(&ocrArgs));
    if (!ocr) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("Can't create OCR with \"%1\" engine: %2").arg(
                ocrEngineInfo.id,
                dpsoGetError()));
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    dpsoSetHotkeysEnabled(true);

    createQActions();

    tabs = new QTabWidget();
    tabs->addTab(createMainTab(), pgettext("ui.tab", "Main"));
    tabs->addTab(createHistoryTab(), _("History"));
    tabs->addTab(createAboutTab(), _("About"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);

    // Our main window may be hidden to tray. As of Qt 5.15, ignoring
    // QCloseEvent has no effect if the window is hidden when
    // closeEvent() returns, even if WA_QuitOnClose is disabled. A
    // workaround is to either disable quitOnLastWindowClosed and
    // quit explicitly when needed (our approach), or to make sure
    // that the window is visible before returning from closeEvent().
    QApplication::setQuitOnLastWindowClosed(false);

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

    // See comments in commitData().
    #if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) \
        && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setFallbackSessionManagementEnabled(false);
    #endif

    connect(
        qApp, SIGNAL(commitDataRequest(QSessionManager&)),
        this, SLOT(commitData(QSessionManager&)),
        Qt::DirectConnection);

    #if UI_TASKBAR_WIN
    taskbar.reset(
        uiTaskbarCreateWin(reinterpret_cast<HWND>(winId())));
    if (!taskbar)
        qWarning("uiTaskbarCreateWin(): %s", dpsoGetError());
    #endif
}


MainWindow::~MainWindow()
{
    dpsoShutdown();
}


void MainWindow::timerEvent(QTimerEvent* event)
{
    (void)event;

    dpsoUpdate();

    // The selection doesn't block keyboard and mouse interaction, so
    // disable it once it no longer makes sense.
    if (dpsoGetSelectionIsEnabled() && !canStartSelection())
        setSelectionIsEnabled(false);

    updateStatus();
    checkResults();
    checkHotkeyActions();
}


static bool confirmQuitWhileOcrIsActive(QWidget* parent)
{
    return confirmDestructiveAction(
        parent,
        _("Recognition is not yet finished. Quit anyway?"),
        _("Cancel"),
        _("Quit"));
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (dpsoOcrHasPendingJobs(ocr.get())
            && !confirmQuitWhileOcrIsActive(this)) {
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
            uiAppName,
            QString("Can't save \"%1\": %2").arg(
                cfgFilePath.c_str(), dpsoGetError()));

    dpsoSetHotkeysEnabled(false);

    event->accept();
    qApp->quit();
}


void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange
            && isMinimized()
            && trayIcon->isVisible()
            && minimizeToTray
            // The tray icon is unresponsive when a modal window is
            // active, so it will be impossible to unhide the
            // application in desktop environments where the modal
            // window is hidden along with the main one.
            && !QApplication::activeModalWidget())
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


void MainWindow::commitData(QSessionManager& sessionManager)
{
    // Our main goal here is to save settings. Still, we also try to
    // use interaction if allowed by the session manager, and here
    // come the problems related to different behavior and bugs in
    // various Qt versions.
    //
    // There are two types of session management:
    //
    // * The fallback management, when after commitData() Qt tries to
    //   close all toplevel windows (invoking closeEvent()), and, if
    //   that fails, assumes that the shutdown process is canceled by
    //   the user.
    //
    // * The "true" management, when Qt only calls commitData() and
    //   no closeEvent().

    // 1. Qt 4 always uses the fallback management. There is no sense
    //    to use commitData(); we don't want to show the confirmation
    //    dialog for the second time from closeEvent().
    //
    //    We no longer support Qt 4, so this case is not relevant.

    bool noInteraction{};
    // 2. Qt version 5.2.1 (and probably all Qt 5 versions before 5.6,
    //    which introduced setFallbackSessionManagementEnabled()) has
    //    a bug: it doesn't call closeEvent(), but instead hides the
    //    window even after QSessionManager::cancel(). The best we can
    //    do here is to disable interaction.
    #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) \
            && QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    noInteraction = true;
    #endif

    // 3. Qt versions from 5.6 till 6.0 allow to disable the fallback
    //    management via setFallbackSessionManagementEnabled(). The
    //    "true" management seem to give a better desktop integration
    //    (see https://bugs.kde.org/show_bug.cgi?id=354724), so we
    //    actually disable it in the constructor.

    // 4. The fallback management is removed in Qt 6. But there's an
    //    issue on Windows 10 with Qt 6.3.0; it's not clear whether
    //    it's a bug in Qt or intended OS behavior. While shutting
    //    down, there is the system screen with a list of apps that
    //    prevent OS from exiting, and two buttons: "Shut down anyway"
    //    and "Cancel". This means that the user can only see our
    //    confirmation dialog after explicitly canceling the shutdown,
    //    which makes the dialog useless (in particular, confirming to
    //    quit at this point will not close the application).
    //
    //    Let's hope that the issue is fixed in the future, but for
    //    now we have to save settings before showing the dialog.

    saveState(cfg.get());
    dpsoCfgSave(cfg.get(), cfgFilePath.c_str());

    if (!noInteraction
            && sessionManager.allowsInteraction()
            && dpsoOcrHasPendingJobs(ocr.get())
            && !confirmQuitWhileOcrIsActive(this)) {
        sessionManager.cancel();
        return;
    }

    sessionManager.release();

    // Terminate jobs in case the session manager actually decides to
    // quit (it may not if e.g. another app cancels shutdown process)
    // so that we don't show the confirmation dialog for the second
    // time from closeEvent() (if the current Qt version use the
    // fallback session management and the checks above didn't help)
    // and don't get partially written files or similar surprises when
    // the app is terminated while processing a new recognized text
    // (if the current Qt version doesn't use the fallback management,
    // and closeEvent(), which stops the update timer, is not called).
    dpsoOcrTerminateJobs(ocr.get());
}


void MainWindow::createQActions()
{
    visibilityAction = new QAction(
        dpsoStrNamedFormat(
            _("Show {app_name}"), {{"app_name", uiAppName}}),
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
    auto* statusGroupLayout = new QHBoxLayout(statusGroup);

    statusIndicator = new StatusIndicator();
    statusGroupLayout->addWidget(statusIndicator);

    statusLabel = new QLabel();
    statusLabel->setTextFormat(Qt::PlainText);
    statusGroupLayout->addWidget(statusLabel, 1);

    auto* ocrGroup = new QGroupBox(_("Character recognition"));
    auto* ocrGroupLayout = new QVBoxLayout(ocrGroup);

    splitTextBlocksCheck = new QCheckBox(
        _("Split text blocks"));
    splitTextBlocksCheck->setToolTip(
        _("Split independent text blocks, such as columns"));
    ocrGroupLayout->addWidget(splitTextBlocksCheck);

    ocrGroupLayout->addWidget(new QLabel(_("Languages:")));
    langBrowser = new LangBrowser(ocr.get());
    ocrGroupLayout->addWidget(langBrowser);

    auto* actionsGroup = new QGroupBox(_("Actions"));
    auto* actionsGroupLayout = new QVBoxLayout(actionsGroup);

    actionChooser = new ActionChooser();
    connect(
        actionChooser, SIGNAL(actionsChanged()),
        this, SLOT(invalidateStatus()));

    actionsGroupLayout->addWidget(actionChooser);

    auto* hotkeyGroup = new QGroupBox(_("Hotkey"));
    auto* hotkeyGroupLayout = new QVBoxLayout(hotkeyGroup);

    hotkeyEditor = new HotkeyEditor(
        hotkeyActionToggleSelection, true);
    connect(
        hotkeyEditor, SIGNAL(changed()),
        hotkeyEditor, SLOT(bind()));
    hotkeyGroupLayout->addWidget(hotkeyEditor);

    auto* tab = new QWidget();
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->addWidget(statusGroup);
    tabLayout->addWidget(ocrGroup);
    tabLayout->addWidget(actionsGroup);
    tabLayout->addWidget(hotkeyGroup);

    return tab;
}


// The purpose of dummy tab widgets in create*Tab() methods is their
// layouts that will add margins around wrapped widgets.


QWidget* MainWindow::createHistoryTab()
{
    // Of course, the data directory is a better place for history,
    // but we keep using the config directory for backward
    // compatibility.
    history = new History(cfgDirPath);

    auto* tab = new QWidget();
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->addWidget(history);

    return tab;
}


QWidget* MainWindow::createAboutTab()
{
    auto* about = new About();

    auto* tab = new QWidget();
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->addWidget(about);

    return tab;
}


void MainWindow::createTrayIcon()
{
    trayIconNormal = getIcon(uiIconNameApp);
    trayIconBusy = getIcon(uiIconNameAppBusy);
    trayIconError = getIcon(uiIconNameAppError);

    trayIcon = new QSystemTrayIcon(trayIconNormal, this);

    auto* menu = new QMenu(this);
    menu->addAction(visibilityAction);
    menu->addAction(quitAction);

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

    selectionBorderWidth = dpsoCfgGetInt(
        cfg,
        cfgKeySelectionBorderWidth,
        dpsoGetSelectionDefaultBorderWidth());
    dpsoSetSelectionBorderWidth(selectionBorderWidth);

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

    dpsoCfgSetInt(
        cfg, cfgKeySelectionBorderWidth, selectionBorderWidth);
}


bool MainWindow::canStartSelection() const
{
    return
        dpsoOcrGetNumActiveLangs(ocr.get()) > 0
        && (ocrAllowQueuing || !dpsoOcrHasPendingJobs(ocr.get()))
        && actionChooser->getSelectedActions();
}


// Calls dpsoSetSelectionIsEnabled() and also manages
// hotkeyActionCancelSelection.
void MainWindow::setSelectionIsEnabled(bool isEnabled)
{
    dpsoSetSelectionIsEnabled(isEnabled);

    if (!isEnabled) {
        dpsoUnbindAction(hotkeyActionCancelSelection);
        return;
    }

    DpsoHotkey toggleSelectionHotkey;
    dpsoFindActionHotkey(
        hotkeyActionToggleSelection, &toggleSelectionHotkey);

    if (cancelSelectionHotkey != toggleSelectionHotkey)
        dpsoBindHotkey(
            &cancelSelectionHotkey, hotkeyActionCancelSelection);
}


void MainWindow::setStatus(Status newStatus, const QString& text)
{
    const auto textWithAppName = joinInLayoutDirection(
        " - ", {text, uiAppName});

    setWindowTitle(
        newStatus == Status::ok ? uiAppName : textWithAppName);

    statusIndicator->setStatus(newStatus);
    statusLabel->setText(text);

    trayIcon->setToolTip(textWithAppName);
    if (newStatus != lastStatus)
        switch (newStatus) {
        case Status::ok:
            trayIcon->setIcon(trayIconNormal);
            break;
        case Status::busy:
            trayIcon->setIcon(trayIconBusy);
            break;
        case Status::warning:
            trayIcon->setIcon(trayIconError);
            break;
        }

    UiTaskbarState tbState{};
    switch (newStatus) {
    case Status::ok:
        tbState = UiTaskbarStateNormal;
        break;
    case Status::busy:
        tbState = UiTaskbarStateProgress;
        break;
    case Status::warning:
        tbState = UiTaskbarStateError;
        break;
    }

    uiTaskbarSetState(taskbar.get(), tbState);

    lastStatus = newStatus;
}


void MainWindow::updateStatus()
{
    DpsoOcrProgress progress;
    dpsoOcrGetProgress(ocr.get(), &progress);

    actionChooser->setEnabled(progress.totalJobs == 0);

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
            totalProgress =
                ((progress.curJob - 1) * 100
                    + progress.curJobProgress)
                / progress.totalJobs;

        setStatus(
            Status::busy,
            dpsoStrNamedFormat(
                progressStatusFmt.c_str(),
                {{"progress",
                        std::to_string(totalProgress).c_str()},
                    {"current_job",
                        std::to_string(
                            progress.curJob).c_str()},
                    {"total_jobs",
                        std::to_string(
                            progress.totalJobs).c_str()}}));

        uiTaskbarSetProgress(taskbar.get(), totalProgress);

        return;
    }

    // Invalidate status when the number of active languages
    // changes from or to 0.
    const auto hasActiveLangs =
        dpsoOcrGetNumActiveLangs(ocr.get()) > 0;
    if (hasActiveLangs != wasActiveLangs) {
        wasActiveLangs = hasActiveLangs;
        statusValid = false;
    }

    if (statusValid)
        return;
    if (dpsoOcrGetNumLangs(ocr.get()) == 0)
        setStatus(Status::warning, _("Please install languages"));
    else if (dpsoOcrGetNumActiveLangs(ocr.get()) == 0)
        setStatus(Status::warning, _("Please select languages"));
    else if (!actionChooser->getSelectedActions())
        setStatus(Status::warning, _("Please select actions"));
    else
        // Translators: Program is ready for OCR
        setStatus(Status::ok, pgettext("ocr.status", "Ready"));

    statusValid = true;
}


void MainWindow::checkResults()
{
    // Check if jobs are completed before fetching results, since new
    // jobs may finish right after dpsoFetchResults() and before
    // dpsoGetJobsPending() call.
    const auto jobsCompleted = !dpsoOcrHasPendingJobs(ocr.get());

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
            dpsoExec(exePath.data(), result.text);
    }

    if (jobsCompleted && clipboardTextPending) {
        QApplication::clipboard()->setText(clipboardText);
        clipboardText.clear();
        clipboardTextPending = false;
    }
}


void MainWindow::checkHotkeyActions()
{
    switch(dpsoGetLastHotkeyAction()) {
    case hotkeyActionToggleSelection: {
        if (!dpsoGetSelectionIsEnabled()) {
            if (canStartSelection())
                setSelectionIsEnabled(true);

            break;
        }

        setSelectionIsEnabled(false);

        DpsoOcrJobArgs jobArgs;

        dpsoGetSelectionGeometry(&jobArgs.screenRect);
        if (dpsoRectIsEmpty(&jobArgs.screenRect))
            break;

        jobArgs.flags = 0;
        if (splitTextBlocksCheck->isChecked())
            jobArgs.flags |= dpsoOcrJobTextSegmentation;

        if (!dpsoOcrQueueJob(ocr.get(), &jobArgs))
            QMessageBox::warning(
                this,
                uiAppName,
                QString("Can't queue OCR job: ") + dpsoGetError());

        break;
    }
    case hotkeyActionCancelSelection:
        setSelectionIsEnabled(false);
        break;
    }
}
