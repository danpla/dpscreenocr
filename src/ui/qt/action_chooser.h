#pragma once

#include <QByteArray>
#include <QFlags>
#include <QWidget>

#include "dpso_ext/dpso_ext.h"


class QCheckBox;
class QLineEdit;


namespace ui::qt {


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

    ActionChooser();

    Actions getSelectedActions() const;
    const char* getExePath() const;

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
    QByteArray exePath;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ActionChooser::Actions)


}
