
#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QTabWidget>
#include <QTimerEvent>
#include <QWidget>

#include "status.h"


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
private slots:
    void chooseExe();
    void invalidateStatus();
private:
    enum HotkeyAction {
        hotkeyActionToggleSelection,
        hotkeyActionCancelSelection,
        numHotkeyActions
    };

    struct DynamicStrings {
        QString chooseExeDialogTitle;
        QString progress;
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

    QTabWidget* tabs;

    StatusIndicator* statusIndicator;
    QLabel* statusLabel;

    QCheckBox* splitTextBlocksCheck;

    LangBrowser* langBrowser;
    HotkeyEditor* hotkeyEditor;

    QCheckBox* copyToClipboardCheck;
    QCheckBox* addToHistoryCheck;
    QCheckBox* runExeCheck;
    QLineEdit* exeLineEdit;

    QString copyToClipboardTextSeparator;
    QString runExeTextSeparator;
    bool sameTextSeparators;

    History* history;

    QWidget* createMainTab();
    QWidget* createActionsTab();
    QWidget* createHistoryTab();
    QWidget* createAboutTab();

    void loadState();
    void initReadOnlyCfgKeys() const;
    void saveState() const;

    bool canStartSelection() const;

    void updateDpso();
    void setStatus(Status newStatus, const QString& text);
    void updateStatus();
    void checkResult();
    void checkHotkeyActions();
};
