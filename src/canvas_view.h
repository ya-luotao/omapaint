#pragma once

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

// Pure view math shared by CanvasItem and its tests: where the image sits in
// item coordinates (centered while it fits, scrolled by pan once it does not)
// and the mapping between item and image coordinates.
namespace CanvasView {

// Top-left corner of the zoomed image in item coordinates.
QPointF origin(const QSizeF &viewport, const QSize &image, qreal zoom,
               const QPointF &pan);

QPointF toImage(const QPointF &itemPos, const QPointF &origin, qreal zoom);
QRectF fromImage(const QRectF &imageRect, const QPointF &origin, qreal zoom);

// Aligns a logical coordinate to the device pixel grid so image pixels and
// grid lines stay crisp under fractional display scaling.
qreal snapToDevicePixels(qreal value, qreal devicePixelRatio);

} // namespace CanvasView
