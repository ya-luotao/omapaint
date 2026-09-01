#pragma once

#include "tool.h"

class LineTool : public ShapeTool
{
public:
    QString name() const override { return QStringLiteral("Line"); }

protected:
    QPen pen(const ToolContext &ctx) const override
    {
        return QPen(ctx.color, ctx.size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }

    void drawShape(QPainter &painter, const QPointF &anchor,
                   const QPointF &pos, const ToolContext &) const override
    {
        if (anchor == pos)
            painter.drawPoint(anchor);
        else
            painter.drawLine(anchor, pos);
    }
};
