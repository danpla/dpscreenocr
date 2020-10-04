
#include "status_indicator.h"

#include <QPainter>


StatusIndicator::StatusIndicator(QWidget* parent)
    : QWidget(parent)
    , status{}
{

}


void StatusIndicator::setStatus(Status newStatus)
{
    if (newStatus == status)
        return;

    status = newStatus;
    update();
}


QSize StatusIndicator::minimumSizeHint() const
{
    const auto size = fontMetrics().ascent();
    return QSize(size, size);
}


static const QColor& getStatusColor(Status status)
{
    static QColor green(0x60, 0xee, 0x60);
    static QColor yellow(0xee, 0xee, 0x60);
    static QColor red(0xee, 0x60, 0x60);

    if (status == Status::ok)
        return green;
    else if (status == Status::busy)
        return yellow;
    else
        return red;
}


void StatusIndicator::paintEvent(QPaintEvent* event)
{
    (void)event;

    const auto& color = getStatusColor(status);

    const auto size = qMin(width(), height());
    const auto strokeWidth = size / 6.0f;
    const auto halfStrokeWidth = strokeWidth * 0.5f;

    const QRectF rect(
        halfStrokeWidth + (width() - size) * 0.5f,
        halfStrokeWidth + (height() - size) * 0.5f,
        size - strokeWidth,
        size - strokeWidth);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color.darker(), strokeWidth));
    painter.setBrush(color);
    painter.drawEllipse(rect);
}
