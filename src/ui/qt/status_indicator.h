
#pragma once

#include <QWidget>

#include "status.h"


class StatusIndicator: public QWidget {
    Q_OBJECT

public:
    explicit StatusIndicator(QWidget* parent = nullptr);

    void setStatus(Status newStatus);

    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    Status status;
};
