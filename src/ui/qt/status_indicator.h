
#pragma once

#include <QWidget>

#include "status.h"


namespace ui::qt {


class StatusIndicator: public QWidget {
    Q_OBJECT
public:
    void setStatus(Status newStatus);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    Status status{};
};


}
