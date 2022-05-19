
#pragma once

#include <string>

#include <QSystemTrayIcon>
#include <QWidget>

#include "dpso/dpso.h"
#include "dpso_utils/dpso_utils.h"

#include "status.h"


class QAction;
class QCheckBox;
class QLabel;
class QSystemTrayIcon;
class QTabWidget;

class ActionChooser;
class History;
class HotkeyEditor;
class LangBrowser;
class StatusIndicator;


class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
protected:
    void timerEvent(QTimerEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
private slots:
    void invalidateStatus();
    void setVisibility(bool vilible);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
private:
    struct DynamicStrings {
        std::string progress;
        QString installLangs;
        QString selectLangs;
        QString selectActions;
        QString ready;
        QString confirmQuitText;
        QString cancel;
        QString quit;

        DynamicStrings();
    } dynStr;

    dpso::OcrUPtr ocr;

    std::string cfgDirPath;
    std::string cfgFilePath;

    dpso::CfgUPtr cfg;

    int updateTimerId;

    bool ocrAllowQueuing;

    DpsoOcrProgress lastProgress;
    bool wasActiveLangs;
    bool statusValid;

    QAction* visibilityAction;
    QAction* quitAction;

    QTabWidget* tabs;

    StatusIndicator* statusIndicator;
    QLabel* statusLabel;

    QCheckBox* splitTextBlocksCheck;

    LangBrowser* langBrowser;
    HotkeyEditor* hotkeyEditor;
    DpsoHotkey cancelSelectionHotkey;

    QWidget* actionsTab;
    ActionChooser* actionChooser;

    QString clipboardText;
    bool clipboardTextPending;
    QString copyToClipboardTextSeparator;

    History* history;

    QSystemTrayIcon* trayIcon;
    bool minimizeToTray;

    bool minimizeOnStart;

    void createQActions();

    QWidget* createMainTab();
    QWidget* createActionsTab();
    QWidget* createHistoryTab();
    QWidget* createAboutTab();

    void createTrayIcon();

    bool loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;

    bool canStartSelection() const;

    void updateDpso();
    void setStatus(Status newStatus, const QString& text);
    void updateStatus();
    void checkResults();
    void checkHotkeyActions();
};
