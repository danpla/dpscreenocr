#pragma once

#include <ctime>
#include <functional>
#include <optional>

#include <QObject>

#include "dpso_ext/dpso_ext.h"
#include "ui_common/ui_common.h"


class QWidget;


namespace ui::qt {


class UpdateChecker : public QObject {
    Q_OBJECT
public:
    // UpdateChecker will not display an update info window while a
    // modal or popup widget is active. The isBusy() function allows
    // you to add additional checks for this case.
    explicit UpdateChecker(
        QWidget* parent, const std::function<bool()>& isBusy = {});

    bool getAutoCheckIsEnabled() const;
    int getAutoCheckIntervalDays() const;

    void loadState(const DpsoCfg* cfg);
    void saveState(DpsoCfg* cfg) const;
public slots:
    void setAutoCheckIsEnabled(bool isEnabled);
    void checkUpdates();
protected:
    void timerEvent(QTimerEvent* event) override;
private:
    QWidget* parentWidget;
    std::function<bool()> isBusy;
    int timerId{};
    UpdateCheckerUPtr autoChecker;
    bool autoCheck{};
    int autoCheckIntervalDays{};
    std::optional<std::tm> lastCheckTime;

    void handleAutoCheck();
    void stopAutoCheck();
};


}
