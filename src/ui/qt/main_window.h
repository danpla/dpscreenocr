#pragma once

#include <optional>
#include <string>

#include <QIcon>
#include <QWidget>

#include "dpso_ext/dpso_ext.h"
#include "dpso_ocr/dpso_ocr.h"
#include "dpso_sys/dpso_sys.h"
#include "ui_common/ui_common.h"

#include "status.h"
#include "update_checker.h"


class QAction;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSessionManager;
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
    void chooseSound();
    void invalidateStatus();
    void setVisibility(bool vilible);
    void commitData(QSessionManager& sessionManager);
private:
    dpso::SysUPtr sys;
    DpsoKeyManager* keyManager;
    DpsoSelection* selection;

    std::string progressStatusFmt;

    dpso::OcrUPtr ocr;

    std::string cfgDirPath;
    std::string cfgFilePath;

    dpso::CfgUPtr cfg;

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

    std::optional<QString> clipboardText;
    QString clipboardTextSeparator;

    History* history;

    HotkeyEditor* hotkeyEditor;
    QCheckBox* showTrayIconCheck;
    QCheckBox* minimizeToTrayCheck;
    QCheckBox* closeToTrayCheck;

    QCheckBox* playSoundCheck;
    QCheckBox* playCustomSoundCheck;
    QLineEdit* customSoundLineEdit;
    QString selectedSoundFilter;

    UpdateChecker updateChecker;
    QCheckBox* autoUpdateCheck;

    ui::AutostartUPtr autostart;
    QCheckBox* autostartCheck;

    QSystemTrayIcon* trayIcon;
    QIcon trayIconNormal;
    QIcon trayIconBusy;
    QIcon trayIconError;

    ui::TaskbarUPtr taskbar;

    bool minimizeOnStart{};
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

    void reloadSound();
    void testSound();

    bool canStartSelection() const;
    void setSelectionIsEnabled(bool isEnabled);

    void setStatus(Status newStatus, const QString& text);
    void updateStatus();
    void checkResults();
    void checkHotkeyActions();
};


}
