
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

#include "dpso/dpso.h"
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
    , wasActiveLangs{}
    , statusValid{}
{
    setWindowTitle(appName);
    QApplication::setWindowIcon(getIcon(appFileName));

    if (!dpsoInit()) {
        QMessageBox::critical(
            nullptr, QString(appName) + " error", dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    const auto* cfgPath = dpsoGetCfgPath(appFileName);
    if (!cfgPath) {
        QMessageBox::critical(
            nullptr,
            QString(appName) + " error",
            QString("Can't get configuration path: ")
                + dpsoGetError());

        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

    cfgDirPath = cfgPath;
    cfgFilePath = cfgDirPath + *dpsoDirSeparators + cfgFileName;

    if (!dpsoCfgLoad(cfgFilePath.c_str())) {
        QMessageBox::critical(
            nullptr,
            QString(appName) + " error",
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

    if (!loadState()) {
        dpsoShutdown();
        std::exit(EXIT_FAILURE);
    }

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
    if (dpsoGetJobsPending()
            && !confirmation(
                this,
                dynStr.confirmQuitText, dynStr.cancel, dynStr.quit)) {
        event->ignore();
        return;
    }

    killTimer(updateTimerId);

    saveState();

    // On some platforms and desktop environments (like KDE), the app
    // keeps working in the background if the tray icon is not hidden
    // before quitting.
    trayIcon->hide();

    if (!dpsoCfgSave(cfgFilePath.c_str()))
        QMessageBox::critical(
            this,
            QString(appName) + " error",
            QString("Can't save \"%1\": %2").arg(
                cfgFilePath.c_str(), dpsoGetError()));

    dpsoSetHotheysEnabled(false);

    event->accept();
}


void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange
        && isMinimized()
        && trayIcon->isVisible()
        && dpsoCfgGetBool(
            cfgKeyUiWindowMinimizeToTray,
            cfgDefaultValueUiWindowMinimizeToTray)) {
        visibilityAction->toggle();
    }

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


bool MainWindow::loadState()
{
    ocrAllowQueuing = dpsoCfgGetBool(
        cfgKeyOcrAllowQueuing, cfgDefaultValueOcrAllowQueuing);

    splitTextBlocksCheck->setChecked(
        dpsoCfgGetBool(
            cfgKeyOcrSplitTextBlocks,
            cfgDefaultValueOcrSplitTextBlocks));

    DpsoHotkey toggleSelectionHotkey;
    dpsoCfgGetHotkey(
        cfgKeyHotkeyToggleSelection,
        &toggleSelectionHotkey,
        &cfgDefaultValueHotkeyToggleSelection);
    dpsoBindHotkey(
        &toggleSelectionHotkey, hotkeyActionToggleSelection);

    hotkeyEditor->assignHotkey();

    copyToClipboardTextSeparator = dpsoCfgGetStr(
        cfgKeyActionCopyToClipboardTextSeparator,
        cfgDefaultValueActionCopyToClipboardTextSeparator);
    runExeWaitToComplete = dpsoCfgGetBool(
        cfgKeyActionRunExecutableWaitToComplete,
        cfgDefaultValueActionRunExecutableWaitToComplete);

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
    if (!history->loadState())
        return false;

    trayIcon->setVisible(
        dpsoCfgGetBool(
            cfgKeyUiTrayIconVisible,
            cfgDefaultValueUiTrayIconVisible));

    return true;
}


void MainWindow::saveState() const
{
    dpsoCfgSetBool(cfgKeyOcrAllowQueuing, ocrAllowQueuing);

    dpsoCfgSetBool(
        cfgKeyOcrSplitTextBlocks,
        splitTextBlocksCheck->isChecked());

    DpsoHotkey toggleSelectionHotkey;
    dpsoFindActionHotkey(
        hotkeyActionToggleSelection, &toggleSelectionHotkey);
    dpsoCfgSetHotkey(
        cfgKeyHotkeyToggleSelection, &toggleSelectionHotkey);

    if (!dpsoCfgKeyExists(cfgKeyHotkeyCancelSelection))
        dpsoCfgSetHotkey(
            cfgKeyHotkeyCancelSelection,
            &cfgDefaultValueHotkeyCancelSelection);

    dpsoCfgSetStr(
        cfgKeyActionCopyToClipboardTextSeparator,
        copyToClipboardTextSeparator.toUtf8().data());
    dpsoCfgSetBool(
        cfgKeyActionRunExecutableWaitToComplete,
        runExeWaitToComplete);

    if (!dpsoCfgKeyExists(cfgKeyUiNativeFileDialogs))
        dpsoCfgSetBool(
            cfgKeyUiNativeFileDialogs,
            cfgDefaultValueUiNativeFileDialogs);

    dpsoCfgSetInt(cfgKeyUiActiveTab, tabs->currentIndex());

    if (!isMaximized()) {
        dpsoCfgSetInt(cfgKeyUiWindowX, x());
        dpsoCfgSetInt(cfgKeyUiWindowY, y());
        dpsoCfgSetInt(cfgKeyUiWindowWidth, width());
        dpsoCfgSetInt(cfgKeyUiWindowHeight, height());
    }

    dpsoCfgSetBool(cfgKeyUiWindowMaximized, isMaximized());

    langBrowser->saveState();
    actionChooser->saveState();
    history->saveState();

    dpsoCfgSetBool(cfgKeyUiTrayIconVisible, trayIcon->isVisible());
    if (!dpsoCfgKeyExists(cfgKeyUiWindowMinimizeToTray))
        dpsoCfgSetBool(
            cfgKeyUiWindowMinimizeToTray,
            cfgDefaultValueUiWindowMinimizeToTray);
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

    DpsoJobResults results;
    dpsoGetFetchedResults(&results);

    const auto actions = actionChooser->getSelectedActions();

    if (actions & ActionChooser::Action::copyToClipboard) {
        clipboardText.clear();

        for (int i = 0; i < results.numItems; ++i) {
            if (i > 0)
                clipboardText += copyToClipboardTextSeparator;

            const auto& result = results.items[i];
            clipboardText += QString::fromUtf8(
                result.text, result.textLen);
        }

        QApplication::clipboard()->setText(clipboardText);
    }

    if (actions & ActionChooser::Action::addToHistory)
        for (int i = 0; i < results.numItems; ++i)
            history->append(
                results.items[i].text, results.items[i].timestamp);

    if (actions & ActionChooser::Action::runExe) {
        const auto exePath = actionChooser->getExePath().toUtf8();

        for (int i = 0; i < results.numItems; ++i)
            dpsoExec(
                exePath.data(),
                results.items[i].text,
                runExeWaitToComplete);
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
            DpsoJobArgs jobArgs{};

            dpsoGetSelectionGeometry(&jobArgs.screenRect);
            if (dpsoRectIsEmpty(&jobArgs.screenRect))
                return;

            jobArgs.flags = 0;
            if (splitTextBlocksCheck->isChecked())
                jobArgs.flags |= dpsoJobTextSegmentation;

            if (!dpsoQueueJob(&jobArgs))
                QMessageBox::warning(
                    this,
                    appName,
                    QString("Can't queue job: ") + dpsoGetError());
        }
    } else if (hotkeyAction == hotkeyActionToggleSelection
            && canStartSelection()) {
        dpsoSetSelectionIsEnabled(true);

        DpsoHotkey cancelSelectionHotkey;
        dpsoCfgGetHotkey(
            cfgKeyHotkeyCancelSelection,
            &cancelSelectionHotkey,
            &cfgDefaultValueHotkeyCancelSelection);

        dpsoBindHotkey(
            &cancelSelectionHotkey, hotkeyActionCancelSelection);
    }
}
