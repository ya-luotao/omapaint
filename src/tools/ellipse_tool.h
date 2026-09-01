#pragma once

#include "tool.h"

class EllipseTool : public ShapeTool
{
public:
    QString name() const override { return QStringLiteral("Ellipse"); }

protected:
    void drawShape(QPainter &painter, const QPointF &anchor,
                   const QPointF &pos, const ToolContext &ctx) const override
    {
        applyShapeFill(painter, ctx);
        painter.drawEllipse(QRectF(anchor, pos).normalized());
    }
};
