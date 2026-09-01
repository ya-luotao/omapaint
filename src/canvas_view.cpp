#include "canvas_view.h"

#include <QtMath>

namespace CanvasView {

namespace {

qreal axisOrigin(qreal viewport, qreal content, qreal pan)
{
    if (content <= viewport)
        return (viewport - content) / 2.0;
    return -pan;
}

} // namespace

QPointF origin(const QSizeF &viewport, const QSize &image, qreal zoom,
               const QPointF &pan)
{
    return QPointF(axisOrigin(viewport.width(), image.width() * zoom, pan.x()),
                   axisOrigin(viewport.height(), image.height() * zoom, pan.y()));
}

QPointF toImage(const QPointF &itemPos, const QPointF &origin, qreal zoom)
{
    return (itemPos - origin) / zoom;
}

QRectF fromImage(const QRectF &imageRect, const QPointF &origin, qreal zoom)
{
    return QRectF(imageRect.topLeft() * zoom + origin, imageRect.size() * zoom);
}

qreal snapToDevicePixels(qreal value, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0)
        return value;
    return qRound(value * devicePixelRatio) / devicePixelRatio;
}

} // namespace CanvasView
