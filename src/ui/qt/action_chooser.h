
#pragma once

#include <QFlags>
#include <QWidget>

#include "dpso_ext/dpso_ext.h"


class QCheckBox;
class QLineEdit;


class ActionChooser : public QWidget {
    Q_OBJECT
public:
    enum Action {
        none = 0,
        copyToClipboard = 1 << 0,
        addToHistory = 1 << 1,
        runExe = 1 << 2
    };

    Q_DECLARE_FLAGS(Actions, Action)

    explicit ActionChooser(QWidget* parent = nullptr);

    Actions getSelectedActions() const;
    QString getExePath() const;

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
signals:
    void actionsChanged();
private slots:
    void chooseExe();
private:
    QCheckBox* copyToClipboardCheck;
    QCheckBox* addToHistoryCheck;
    QCheckBox* runExeCheck;
    QLineEdit* exeLineEdit;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ActionChooser::Actions)
