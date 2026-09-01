#include "tool.h"

#include <QtMath>

QRect Tool::begin(QImage &image, const QPointF &pos, const QColor &color)
{
    m_last = pos;
    return drawSegment(image, pos, pos, color);
}

QRect Tool::move(QImage &image, const QPointF &pos, const QColor &color)
{
    const QRect damage = drawSegment(image, m_last, pos, color);
    m_last = pos;
    return damage;
}

void Tool::end()
{
    m_last = QPointF();
}

QRect Tool::drawSegment(QImage &image, const QPointF &from, const QPointF &to,
                        const QColor &color) const
{
    const QPen p = pen(color);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setCompositionMode(compositionMode());
    painter.setPen(p);
    // A zero-length line is not stroked for wide pens; draw a dot instead.
    if (from == to)
        painter.drawPoint(from);
    else
        painter.drawLine(from, to);
    painter.end();

    const int margin = qCeil(p.widthF() / 2.0) + 1;
    return QRectF(from, to)
        .normalized()
        .adjusted(-margin, -margin, margin, margin)
        .toAlignedRect()
        .intersected(image.rect());
}
