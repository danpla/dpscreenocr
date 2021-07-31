
#pragma once

#include <string>

#include <QSystemTrayIcon>
#include <QWidget>

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

    int updateTimerId;

    bool ocrAllowQueuing;

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

    QWidget* actionsTab;
    ActionChooser* actionChooser;

    QString copyToClipboardTextSeparator;
    bool runExeWaitToComplete;

    History* history;

    QSystemTrayIcon* trayIcon;

    void createQActions();

    QWidget* createMainTab();
    QWidget* createActionsTab();
    QWidget* createHistoryTab();
    QWidget* createAboutTab();

    void createTrayIcon();

    void loadState();
    void saveState() const;

    bool canStartSelection() const;

    void updateDpso();
    void setStatus(Status newStatus, const QString& text);
    void updateStatus();
    void checkResult();
    void checkHotkeyActions();
};
