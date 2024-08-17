
#pragma once

#include <string>

#include <QIcon>
#include <QWidget>

#include "dpso/dpso.h"
#include "dpso_ext/dpso_ext.h"
#include "ui_common/ui_common.h"

#include "status.h"


class QAction;
class QCheckBox;
class QLabel;
class QPushButton;
class QSessionManager;
class QSystemTrayIcon;
class QSystemTrayIcon;
class QTabWidget;


namespace ui::qt {


class ActionChooser;
class History;
class HotkeyEditor;
class LangBrowser;
class StatusIndicator;


class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(const UiStartupArgs& startupArgs);
protected:
    void timerEvent(QTimerEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
private slots:
    void openLangManager();
    void invalidateStatus();
    void setVisibility(bool vilible);
    void commitData(QSessionManager& sessionManager);
private:
    dpso::DpsoInitializer dpsoInitializer;

    std::string progressStatusFmt;

    dpso::OcrUPtr ocr;

    std::string cfgDirPath;
    std::string cfgFilePath;

    dpso::CfgUPtr cfg;

    std::string ocrDataDirPath;

    int updateTimerId{};

    bool ocrAllowQueuing{};

    DpsoOcrProgress lastProgress{};
    bool wasActiveLangs{};
    bool statusValid{};

    QAction* visibilityAction;
    QAction* quitAction;
    bool quitRequested{};

    QTabWidget* tabs;

    Status lastStatus{};
    StatusIndicator* statusIndicator;
    QLabel* statusLabel;

    QCheckBox* splitTextBlocksCheck;

    LangBrowser* langBrowser;
    QPushButton* langManagerButton;
    ActionChooser* actionChooser;
    DpsoHotkey cancelSelectionHotkey{};

    QString clipboardText;
    bool clipboardTextPending{};
    QString copyToClipboardTextSeparator;

    History* history;

    HotkeyEditor* hotkeyEditor;
    QCheckBox* startMinimizedCheck;
    QCheckBox* showTrayIconCheck;
    QCheckBox* minimizeToTrayCheck;
    QCheckBox* closeToTrayCheck;

    ui::AutostartUPtr autostart;
    QCheckBox* autostartCheck;

    QSystemTrayIcon* trayIcon;
    QIcon trayIconNormal;
    QIcon trayIconBusy;
    QIcon trayIconError;

    ui::TaskbarUPtr taskbar;

    int selectionBorderWidth{};

    void createQActions();

    QWidget* createMainTab();
    QWidget* createActionsTab();
    QWidget* createHistoryTab();
    QWidget* createSettingsTab();
    QWidget* createAboutTab();

    void createTrayIcon();

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;

    bool canStartSelection() const;
    void setSelectionIsEnabled(bool isEnabled);

    void setStatus(Status newStatus, const QString& text);
    void updateStatus();
    void checkResults();
    void checkHotkeyActions();
};


}
