#pragma once

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRect>
#include <QString>

// A stroke tool paints directly into the document image while the pointer is
// down and reports the damaged rect of each segment. The canvas accumulates
// the damage and commits one DrawCommand per stroke on release.
class Tool
{
public:
    virtual ~Tool() = default;

    virtual QString name() const = 0;

    QRect begin(QImage &image, const QPointF &pos, const QColor &color);
    QRect move(QImage &image, const QPointF &pos, const QColor &color);
    void end();

protected:
    virtual QPen pen(const QColor &color) const = 0;
    virtual QPainter::CompositionMode compositionMode() const
    {
        return QPainter::CompositionMode_SourceOver;
    }

private:
    QRect drawSegment(QImage &image, const QPointF &from, const QPointF &to,
                      const QColor &color) const;

    QPointF m_last;
};
