
#include "status_indicator.h"

#include <QPainter>


StatusIndicator::StatusIndicator(QWidget* parent)
    : QWidget(parent)
    , status {}
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


void StatusIndicator::paintEvent(QPaintEvent * event)
{
    (void)event;

    static QColor green(0x60, 0xee, 0x60);
    static QColor yellow(0xee, 0xee, 0x60);
    static QColor red(0xee, 0x60, 0x64);

    QColor* color;
    if (status == Status::ok)
        color = &green;
    else if (status == Status::busy)
        color = &yellow;
    else
        color = &red;

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
    painter.setPen(QPen(color->darker(200), strokeWidth));
    painter.setBrush(*color);
    painter.drawEllipse(rect);
}
