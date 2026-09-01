#pragma once

#include "tool.h"

class RectangleTool : public ShapeTool
{
public:
    QString name() const override { return QStringLiteral("Rectangle"); }

protected:
    void drawShape(QPainter &painter, const QPointF &anchor,
                   const QPointF &pos) const override
    {
        painter.drawRect(QRectF(anchor, pos).normalized());
    }
};
