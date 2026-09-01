#pragma once

#include "tool.h"

class EllipseTool : public ShapeTool
{
public:
    QString name() const override { return QStringLiteral("Ellipse"); }

protected:
    void drawShape(QPainter &painter, const QPointF &anchor,
                   const QPointF &pos) const override
    {
        painter.drawEllipse(QRectF(anchor, pos).normalized());
    }
};
