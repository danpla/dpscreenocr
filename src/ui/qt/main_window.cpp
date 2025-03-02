#include "main_window.h"

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
#include <QPushButton>
#include <QSessionManager>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QtGlobal>
#include <QtGlobal>

#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "about.h"
#include "action_chooser.h"
#include "error.h"
#include "history.h"
#include "hotkey_editor.h"
#include "lang_browser.h"
#include "lang_manager/lang_manager.h"
#include "status_indicator.h"
#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt {


enum {
    hotkeyActionToggleSelection,
    hotkeyActionCancelSelection
};


// Our GUI currently doesn't allow selecting an OCR engine.
const auto ocrEngineIdx = 0;


MainWindow::MainWindow(const UiStartupArgs& startupArgs)
    : progressStatusFmt{_(
        "Recognition {progress}% ({current_job}/{total_jobs})")}
    , updateChecker{
        this,
        [&]{ return dpsoSelectionGetIsEnabled(selection); }}
{
    setWindowTitle(uiAppName);

    sys.reset(dpsoSysCreate());
    if (!sys)
        throw Error(
            std::string("Can't create system backend: ")
            + dpsoGetError());

    keyManager = dpsoSysGetKeyManager(sys.get());
    selection = dpsoSysGetSelection(sys.get());

    const auto* cfgPath = dpsoGetUserDir(
        DpsoUserDirConfig, uiAppFileName);
    if (!cfgPath)
        throw Error(
            std::string("Can't get configuration path: ")
            + dpsoGetError());

    cfgDirPath = cfgPath;
    cfgFilePath = cfgDirPath + *dpsoDirSeparators + uiCfgFileName;

    cfg.reset(dpsoCfgCreate());
    if (!cfg)
        throw Error(
            std::string("Can't create Cfg: ") + dpsoGetError());

    if (!dpsoCfgLoad(cfg.get(), cfgFilePath.c_str()))
        throw Error(
            std::string("Can't load \"")
            + cfgFilePath
            + "\": "
            + dpsoGetError());

    const auto* dataPath = dpsoGetUserDir(
        DpsoUserDirData, uiAppFileName);
    if (!dataPath)
        throw Error(
            std::string("Can't get data path: ") + dpsoGetError());

    if (dpsoOcrGetNumEngines() == 0)
        throw Error("No OCR engines are available");

    DpsoOcrEngineInfo ocrEngineInfo;
    dpsoOcrGetEngineInfo(ocrEngineIdx, &ocrEngineInfo);

    if (const char* dirName = uiGetOcrDataDirName(&ocrEngineInfo);
            *dirName)
        ocrDataDirPath =
            std::string{dataPath} + *dpsoDirSeparators + dirName;

    ocr.reset(dpsoOcrCreate(ocrEngineIdx, ocrDataDirPath.c_str()));
    if (!ocr)
        throw Error(
            std::string("Can't create OCR with \"")
            + ocrEngineInfo.id
            + "\" engine: "
            + dpsoGetError());

    dpsoKeyManagerSetIsEnabled(keyManager, true);

    autostart.reset(uiAutostartCreateDefault());
    if (!autostart)
        throw Error(
            std::string("Can't create autostart handler: ")
            + dpsoGetError());

    createQActions();

    tabs = new QTabWidget();
    tabs->addTab(createMainTab(), pgettext("ui_tab", "Main"));
    tabs->addTab(createHistoryTab(), _("History"));
    tabs->addTab(createSettingsTab(), _("Settings"));
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

    loadState(cfg.get());

    if (!minimizeOnStart && !startupArgs.hide)
        show();
    else if (trayIcon->isVisible()
            && (minimizeToTrayCheck->isChecked()
                    || startupArgs.hide))
        visibilityAction->setChecked(false);
    else
        showMinimized();

    updateTimerId = startTimer(1000 / 60);

    // See comments in commitData().
    #if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) \
        && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setFallbackSessionManagementEnabled(false);
    #endif

    connect(
        qApp, &QGuiApplication::commitDataRequest,
        this, &MainWindow::commitData,
        Qt::DirectConnection);

    #if UI_TASKBAR_WIN
    taskbar.reset(
        uiTaskbarCreateWin(reinterpret_cast<HWND>(winId())));
    if (!taskbar)
        qWarning("uiTaskbarCreateWin(): %s", dpsoGetError());
    #endif
}


void MainWindow::timerEvent(QTimerEvent* event)
{
    (void)event;

    dpsoSysUpdate(sys.get());

    // The selection doesn't block keyboard and mouse interaction, so
    // disable it once it no longer makes sense.
    if (dpsoSelectionGetIsEnabled(selection) && !canStartSelection())
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
    if (!quitRequested
            && trayIcon->isVisible()
            && closeToTrayCheck->isChecked()) {
        visibilityAction->setChecked(false);
        event->ignore();
        return;
    }

    quitRequested = false;

    if (dpsoOcrHasPendingResults(ocr.get())
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

    dpsoKeyManagerSetIsEnabled(keyManager, false);

    event->accept();
    qApp->quit();
}


void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange
            && isMinimized()
            && trayIcon->isVisible()
            && minimizeToTrayCheck->isChecked()
            // The tray icon is unresponsive when a modal window is
            // active, so it will be impossible to unhide the
            // application in desktop environments where the modal
            // window is hidden along with the main one.
            && !QApplication::activeModalWidget())
        visibilityAction->setChecked(false);

    QWidget::changeEvent(event);
}


void MainWindow::openLangManager()
{
    setSelectionIsEnabled(false);
    dpsoKeyManagerSetIsEnabled(keyManager, false);

    langManager::runLangManager(this, ocrEngineIdx, ocrDataDirPath);
    langBrowser->reloadLangs();

    // Refresh status in case we installed the first (or removed the
    // last) language.
    invalidateStatus();

    dpsoKeyManagerSetIsEnabled(keyManager, true);
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
    #if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    noInteraction = true;
    #endif

    // 3. Qt versions from 5.6 till 6.0 allows disabling the fallback
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
            && dpsoOcrHasPendingResults(ocr.get())
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
        strNFormat(_("Show {app_name}"), {{"app_name", uiAppName}}),
        this);
    visibilityAction->setCheckable(true);
    visibilityAction->setChecked(true);
    connect(
        visibilityAction, &QAction::toggled,
        this, &MainWindow::setVisibility);

    quitAction = new QAction(_("Quit"), this);
    connect(
        quitAction,
        &QAction::triggered,
        [&]
        {
            quitRequested = true;
            close();
        });
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

    langManagerButton = new QPushButton(_("Language manager"));
    langManagerButton->setToolTip(
        _("Install, update, and remove languages"));

    DpsoOcrEngineInfo ocrEngineInfo;
    dpsoOcrGetEngineInfo(ocrEngineIdx, &ocrEngineInfo);
    langManagerButton->setVisible(ocrEngineInfo.hasLangManager);

    connect(
        langManagerButton, &QPushButton::clicked,
        this, &MainWindow::openLangManager);
    ocrGroupLayout->addWidget(langManagerButton);

    auto* actionsGroup = new QGroupBox(_("Actions"));
    auto* actionsGroupLayout = new QVBoxLayout(actionsGroup);

    actionChooser = new ActionChooser();
    connect(
        actionChooser, &ActionChooser::actionsChanged,
        this, &MainWindow::invalidateStatus);

    actionsGroupLayout->addWidget(actionChooser);

    auto* tab = new QWidget();
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->addWidget(statusGroup);
    tabLayout->addWidget(ocrGroup);
    tabLayout->addWidget(actionsGroup);

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


QWidget* MainWindow::createSettingsTab()
{
    auto* hotkeyGroup = new QGroupBox(_("Hotkey"));
    auto* hotkeyGroupLayout = new QVBoxLayout(hotkeyGroup);

    hotkeyEditor = new HotkeyEditor(
        keyManager, hotkeyActionToggleSelection, true);
    connect(
        hotkeyEditor, &HotkeyEditor::changed,
        hotkeyEditor, &HotkeyEditor::bind);
    connect(
        hotkeyEditor, &HotkeyEditor::changed,
        this, &MainWindow::invalidateStatus);
    hotkeyGroupLayout->addWidget(hotkeyEditor);

    auto* interfaceGroup = new QGroupBox(_("Interface"));
    auto* interfaceGroupLayout = new QVBoxLayout(interfaceGroup);

    showTrayIconCheck = new QCheckBox(
        _("Show notification area icon"));
    connect(
        showTrayIconCheck, &QCheckBox::toggled,
        [&](bool isChecked)
        {
            trayIcon->setVisible(isChecked);

            if (!isChecked) {
                visibilityAction->setChecked(true);
                minimizeToTrayCheck->setChecked(false);
                closeToTrayCheck->setChecked(false);
            }

            minimizeToTrayCheck->setEnabled(isChecked);
            closeToTrayCheck->setEnabled(isChecked);
        });
    interfaceGroupLayout->addWidget(showTrayIconCheck);

    auto* trayIconSubordinateLayout = new QVBoxLayout();
    trayIconSubordinateLayout->setContentsMargins(
        makeSubordinateControlMargins());
    interfaceGroupLayout->addLayout(trayIconSubordinateLayout);

    minimizeToTrayCheck = new QCheckBox(
        _("Minimize to notification area"));
    minimizeToTrayCheck->setToolTip(
        strNFormat(
            _(
                "Minimizing the window will hide {app_name}. Use the "
                "notification area icon to show it again."),
            {{"app_name", uiAppName}}));
    minimizeToTrayCheck->setEnabled(false);
    trayIconSubordinateLayout->addWidget(minimizeToTrayCheck);

    closeToTrayCheck = new QCheckBox(_("Close to notification area"));
    closeToTrayCheck->setToolTip(
        strNFormat(
            _(
                "Closing the window will hide {app_name}. Use the "
                "notification area icon to show it again."),
            {{"app_name", uiAppName}}));
    closeToTrayCheck->setEnabled(false);
    trayIconSubordinateLayout->addWidget(closeToTrayCheck);

    auto* behaviorGroup = new QGroupBox(_("Behavior"));
    auto* behaviorGroupLayout = new QVBoxLayout(behaviorGroup);

    playSoundCheck = new QCheckBox(
        _("Play sound when recognition is complete"));
    playSoundCheck->setVisible(uiSoundIsAvailable());
    connect(
        // Use clicked() instead of toggled() because this should only
        // be triggered by an interactive activation.
        playSoundCheck, &QCheckBox::clicked,
        [&](bool isChecked)
        {
            if (!isChecked || uiSoundPlay(UiSoundIdDone))
                return;

            playSoundCheck->setChecked(false);
            QMessageBox::critical(
                this,
                uiAppName,
                QString("Can't play sound: ") + dpsoGetError());
        });
    behaviorGroupLayout->addWidget(playSoundCheck);

    autoUpdateCheck = new QCheckBox(
        _("Check for updates automatically"));
    connect(
        autoUpdateCheck, &QCheckBox::toggled,
        &updateChecker, &UpdateChecker::setAutoCheckIsEnabled);
    behaviorGroupLayout->addWidget(autoUpdateCheck);

    autostartCheck = new QCheckBox(_("Run at system logon"));
    autostartCheck->setToolTip(
        strNFormat(
            _(
                "{app_name} will start automatically when you log on "
                "to the system. The program will be hidden to the "
                "notification area if the notification area icon is "
                "enabled, or minimized if the icon is disabled."),
            {{"app_name", uiAppName}}));
    Q_ASSERT(autostart);
    autostartCheck->setChecked(
        uiAutostartGetIsEnabled(autostart.get()));
    connect(
        autostartCheck, &QCheckBox::toggled,
        [&](bool isChecked)
        {
            if (uiAutostartSetIsEnabled(autostart.get(), isChecked))
                return;

            // uiAutostartSetIsEnabled() failure will not change the
            // autostart state, so setChecked() will not result in an
            // infinite recursion as the next lambda call will be
            // no-op.
            autostartCheck->setChecked(
                uiAutostartGetIsEnabled(autostart.get()));

            QMessageBox::critical(
                this,
                uiAppName,
                QString("Can't %1 autostart: %2").arg(
                    isChecked ? "enable" : "disable",
                    dpsoGetError()));
        });
    behaviorGroupLayout->addWidget(autostartCheck);

    auto* tab = new QWidget();
    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->addWidget(hotkeyGroup);
    tabLayout->addWidget(interfaceGroup);
    tabLayout->addWidget(behaviorGroup);
    tabLayout->addStretch();

    return tab;
}


QWidget* MainWindow::createAboutTab()
{
    auto* about = new About();
    connect(
        about, &About::checkUpdates,
        &updateChecker, &UpdateChecker::checkUpdates);

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
        &QSystemTrayIcon::activated,
        [&](QSystemTrayIcon::ActivationReason reason)
        {
            if (reason == QSystemTrayIcon::Trigger)
                visibilityAction->toggle();
        });
}


void MainWindow::loadState(const DpsoCfg* cfg)
{
    ocrAllowQueuing = dpsoCfgGetBool(
        cfg, cfgKeyOcrAllowQueuing, cfgDefaultValueOcrAllowQueuing);

    splitTextBlocksCheck->setChecked(
        dpsoCfgGetBool(
            cfg,
            cfgKeyOcrSplitTextBlocks,
            cfgDefaultValueOcrSplitTextBlocks));

    clipboardTextSeparator = dpsoCfgGetStr(
        cfg,
        cfgKeyActionCopyToClipboardTextSeparator,
        cfgDefaultValueActionCopyToClipboardTextSeparator);

    DpsoHotkey toggleSelectionHotkey;
    dpsoCfgGetHotkey(
        cfg,
        cfgKeyHotkeyToggleSelection,
        &toggleSelectionHotkey,
        &cfgDefaultValueHotkeyToggleSelection);
    dpsoKeyManagerBindHotkey(
        keyManager,
        &toggleSelectionHotkey,
        hotkeyActionToggleSelection);

    hotkeyEditor->assignHotkey();

    dpsoCfgGetHotkey(
        cfg,
        cfgKeyHotkeyCancelSelection,
        &cancelSelectionHotkey,
        &cfgDefaultValueHotkeyCancelSelection);

    trayIcon->setVisible(
        dpsoCfgGetBool(
            cfg,
            cfgKeyUiTrayIconVisible,
            cfgDefaultValueUiTrayIconVisible));
    showTrayIconCheck->setChecked(trayIcon->isVisible());
    minimizeToTrayCheck->setChecked(
        trayIcon->isVisible()
        && dpsoCfgGetBool(
            cfg,
            cfgKeyUiWindowMinimizeToTray,
            cfgDefaultValueUiWindowMinimizeToTray));
    closeToTrayCheck->setChecked(
        trayIcon->isVisible()
        && dpsoCfgGetBool(
            cfg,
            cfgKeyUiWindowCloseToTray,
            cfgDefaultValueUiWindowCloseToTray));

    playSoundCheck->setChecked(
        uiSoundIsAvailable()
        && dpsoCfgGetBool(
            cfg,
            cfgKeyActionsDonePlaySound,
            cfgDefaultValueActionsDonePlaySound));
    playCustomSound = dpsoCfgGetBool(
        cfg,
        cfgKeyActionsDonePlaySoundCustom,
        cfgDefaultValueActionsDonePlaySoundCustom);
    playCustomSoundPath = dpsoCfgGetStr(
        cfg, cfgKeyActionsDonePlaySoundCustomPath, "");
    uiSoundSetFilePath(
        UiSoundIdDone,
        playCustomSound ? playCustomSoundPath.c_str() : "");

    updateChecker.loadState(cfg);
    autoUpdateCheck->setChecked(
        updateChecker.getAutoCheckIsEnabled());
    autoUpdateCheck->setToolTip(
        strNFormat(
            ngettext(
                "{app_name} will automatically check for updates at "
                "startup, but no more often than once every {count} "
                "day",
                "{app_name} will automatically check for updates at "
                "startup, but no more often than once every {count} "
                "days",
                updateChecker.getAutoCheckIntervalDays()),
            {
                {"app_name", uiAppName},
                {"count", updateChecker.getAutoCheckIntervalDays()}
            }));

    tabs->setCurrentIndex(dpsoCfgGetInt(cfg, cfgKeyUiActiveTab, 0));

    const QSize windowSize{
        dpsoCfgGetInt(cfg, cfgKeyUiWindowWidth, 0),
        dpsoCfgGetInt(cfg, cfgKeyUiWindowHeight, 0)};

    if (!windowSize.isEmpty()) {
        move(
            dpsoCfgGetInt(cfg, cfgKeyUiWindowX, x()),
            dpsoCfgGetInt(cfg, cfgKeyUiWindowY, y()));
        resize(windowSize);
    }

    if (dpsoCfgGetBool(cfg, cfgKeyUiWindowMaximized, false))
        setWindowState(windowState() | Qt::WindowMaximized);

    langBrowser->loadState(cfg);
    actionChooser->loadState(cfg);
    history->loadState(cfg);

    minimizeOnStart = dpsoCfgGetBool(
        cfg,
        cfgKeyUiWindowMinimizeOnStart,
        cfgDefaultValueUiWindowMinimizeOnStart);

    selectionBorderWidth = dpsoCfgGetInt(
        cfg,
        cfgKeySelectionBorderWidth,
        dpsoSelectionGetDefaultBorderWidth());
    dpsoSelectionSetBorderWidth(selection, selectionBorderWidth);
}


void MainWindow::saveState(DpsoCfg* cfg) const
{
    dpsoCfgSetBool(cfg, cfgKeyOcrAllowQueuing, ocrAllowQueuing);

    dpsoCfgSetBool(
        cfg,
        cfgKeyOcrSplitTextBlocks,
        splitTextBlocksCheck->isChecked());

    dpsoCfgSetStr(
        cfg,
        cfgKeyActionCopyToClipboardTextSeparator,
        clipboardTextSeparator.toUtf8().data());

    DpsoHotkey toggleSelectionHotkey;
    dpsoKeyManagerFindActionHotkey(
        keyManager,
        hotkeyActionToggleSelection,
        &toggleSelectionHotkey);
    dpsoCfgSetHotkey(
        cfg, cfgKeyHotkeyToggleSelection, &toggleSelectionHotkey);

    dpsoCfgSetHotkey(
        cfg, cfgKeyHotkeyCancelSelection, &cancelSelectionHotkey);

    dpsoCfgSetBool(
        cfg, cfgKeyUiTrayIconVisible, trayIcon->isVisible());
    dpsoCfgSetBool(
        cfg,
        cfgKeyUiWindowMinimizeToTray,
        minimizeToTrayCheck->isChecked());
    dpsoCfgSetBool(
        cfg,
        cfgKeyUiWindowCloseToTray,
        closeToTrayCheck->isChecked());

    dpsoCfgSetBool(
        cfg, cfgKeyActionsDonePlaySound, playSoundCheck->isChecked());
    dpsoCfgSetBool(
        cfg, cfgKeyActionsDonePlaySoundCustom, playCustomSound);
    dpsoCfgSetStr(
        cfg,
        cfgKeyActionsDonePlaySoundCustomPath,
        playCustomSoundPath.c_str());

    updateChecker.saveState(cfg);

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
        cfg, cfgKeyUiWindowMinimizeOnStart, minimizeOnStart);

    dpsoCfgSetInt(
        cfg, cfgKeySelectionBorderWidth, selectionBorderWidth);
}


bool MainWindow::canStartSelection() const
{
    return
        dpsoOcrGetNumActiveLangs(ocr.get()) > 0
        && (ocrAllowQueuing || !dpsoOcrHasPendingResults(ocr.get()))
        && actionChooser->getSelectedActions();
}


// Calls dpsoSelectionSetIsEnabled() and also manages
// hotkeyActionCancelSelection.
void MainWindow::setSelectionIsEnabled(bool isEnabled)
{
    dpsoSelectionSetIsEnabled(selection, isEnabled);
    invalidateStatus();

    if (!isEnabled) {
        dpsoKeyManagerUnbindAction(
            keyManager, hotkeyActionCancelSelection);
        return;
    }

    DpsoHotkey toggleSelectionHotkey;
    dpsoKeyManagerFindActionHotkey(
        keyManager,
        hotkeyActionToggleSelection,
        &toggleSelectionHotkey);

    if (cancelSelectionHotkey != toggleSelectionHotkey)
        dpsoKeyManagerBindHotkey(
            keyManager,
            &cancelSelectionHotkey,
            hotkeyActionCancelSelection);
}


void MainWindow::setStatus(Status newStatus, const QString& text)
{
    const auto title = newStatus == Status::ok
        ? uiAppName : joinInLayoutDirection(" - ", {text, uiAppName});

    setWindowTitle(title);

    statusIndicator->setStatus(newStatus);
    statusLabel->setText(text);

    trayIcon->setToolTip(title);
    if (newStatus != lastStatus)
        switch (newStatus) {
        case Status::ok:
            trayIcon->setIcon(trayIconNormal);
            break;
        case Status::busy:
            trayIcon->setIcon(trayIconBusy);
            break;
        case Status::error:
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
    case Status::error:
        tbState = UiTaskbarStateError;
        break;
    }

    uiTaskbarSetState(taskbar.get(), tbState);

    lastStatus = newStatus;
}


void MainWindow::updateStatus()
{
    const auto resultsPending = dpsoOcrHasPendingResults(ocr.get());

    actionChooser->setEnabled(!resultsPending);
    langManagerButton->setEnabled(!resultsPending);

    if (resultsPending) {
        // Update the status when we are done with the results.
        statusValid = false;

        DpsoOcrProgress progress;
        dpsoOcrGetProgress(ocr.get(), &progress);
        if (progress.totalJobs == 0)
            // If the recognition is complete, but the results have
            // not yet been processed, leave the status as it is,
            // which effectively preserves the last progress status
            // (unless the recognition finishes extremely quickly).
            // There's no point in bothering with a "Processing
            // actions..." status text: it will disappear almost
            // immediately, since the next checkResults() will process
            // all results.
            return;

        if (lastStatus == Status::busy
                && dpsoOcrProgressEqual(&progress, &lastProgress))
            return;

        lastProgress = progress;

        const auto totalProgress = progress.curJob == 0
            ? 0
            : (((progress.curJob - 1) * 100 + progress.curJobProgress)
                / progress.totalJobs);

        setStatus(
            Status::busy,
            strNFormat(
                progressStatusFmt.c_str(),
                {
                    {"progress", totalProgress},
                    {"current_job", progress.curJob},
                    {"total_jobs", progress.totalJobs}}));

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
        setStatus(Status::error, _("Please install languages"));
    else if (dpsoOcrGetNumActiveLangs(ocr.get()) == 0)
        setStatus(Status::error, _("Please select languages"));
    else if (!actionChooser->getSelectedActions())
        setStatus(Status::error, _("Please select actions"));
    else {
        DpsoHotkey toggleSelectionHotkey;
        dpsoKeyManagerFindActionHotkey(
            keyManager,
            hotkeyActionToggleSelection,
            &toggleSelectionHotkey);

        setStatus(
            Status::ok,
            strNFormat(
                !dpsoSelectionGetIsEnabled(selection)
                    ? _("Press {hotkey} to start area selection")
                    : _("Press {hotkey} to finish selection"),
                {
                    {
                        "hotkey",
                        dpsoHotkeyToString(&toggleSelectionHotkey)}
                }));
    }

    statusValid = true;
}


void MainWindow::checkResults()
{
    const auto actions = actionChooser->getSelectedActions();

    bool wasResults{};
    DpsoOcrJobResult result;
    while (dpsoOcrGetResult(ocr.get(), &result)) {
        wasResults = true;

        if (actions & ActionChooser::Action::copyToClipboard) {
            if (clipboardText)
                *clipboardText += clipboardTextSeparator;
            else
                clipboardText = QString();

            *clipboardText += QString::fromUtf8(
                result.text, result.textLen);
        }

        if (actions & ActionChooser::Action::addToHistory)
            history->append(result.timestamp, result.text);

        if (actions & ActionChooser::Action::runExe)
            dpsoExec(actionChooser->getExePath(), &result.text, 1);
    }

    if (!wasResults || dpsoOcrHasPendingResults(ocr.get()))
        return;

    if (clipboardText) {
        QApplication::clipboard()->setText(*clipboardText);
        clipboardText.reset();
    }

    if (playSoundCheck->isChecked())
        uiSoundPlay(UiSoundIdDone);
}


void MainWindow::checkHotkeyActions()
{
    switch (dpsoKeyManagerGetLastHotkeyAction(keyManager)) {
    case hotkeyActionToggleSelection: {
        if (!dpsoSelectionGetIsEnabled(selection)) {
            if (canStartSelection())
                setSelectionIsEnabled(true);

            break;
        }

        setSelectionIsEnabled(false);

        DpsoRect selectionRect;
        dpsoSelectionGetGeometry(selection, &selectionRect);
        if (dpsoRectIsEmpty(&selectionRect))
            break;

        auto* screenshot = dpsoTakeScreenshot(
            sys.get(), &selectionRect);
        if (!screenshot) {
            QMessageBox::warning(
                this,
                uiAppName,
                QString("Can't take screenshot: ") + dpsoGetError());
            break;
        }

        DpsoOcrJobFlags flags{};
        if (splitTextBlocksCheck->isChecked())
            flags |= dpsoOcrJobTextSegmentation;

        if (!dpsoOcrQueueJob(ocr.get(), &screenshot, flags))
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


}
