#include "status_indicator.h"

#include <algorithm>
#include <cmath>

#include <QPainter>


namespace ui::qt {


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


QSize StatusIndicator::sizeHint() const
{
    return minimumSizeHint();
}


static const QColor& getStatusColor(Status status)
{
    static const auto a = 0xee;
    static const auto b = 0x60;

    static QColor green(b, a, b);
    static QColor yellow(a, a, b);
    static QColor red(a, b, b);

    switch (status) {
    case Status::ok:
        return green;
    case Status::busy:
        return yellow;
    case Status::error:
        return red;
    }

    Q_ASSERT(false);
    return green;
}


void StatusIndicator::paintEvent(QPaintEvent* /*event*/)
{
    const auto& color = getStatusColor(status);

    const auto size = std::min(width(), height());
    const auto strokeWidth = size / 6.0f;
    const auto halfStrokeWidth = strokeWidth * 0.5f;

    const QRectF rect(
        // We round coordinates to snap the rect to the pixel grid.
        halfStrokeWidth + std::round((width() - size) * 0.5f),
        halfStrokeWidth + std::round((height() - size) * 0.5f),
        size - strokeWidth,
        size - strokeWidth);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color.darker(), strokeWidth));
    painter.setBrush(color);
    painter.drawEllipse(rect);
}


}
