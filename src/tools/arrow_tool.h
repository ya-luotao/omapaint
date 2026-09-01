#pragma once

#include "tool.h"

#include <QtMath>

// Line with a solid triangular head at the release end. The head scales with
// the stroke width.
class ArrowTool : public ShapeTool
{
public:
    QString name() const override { return QStringLiteral("Arrow"); }

protected:
    QPen pen(const ToolContext &ctx) const override
    {
        return QPen(ctx.color, ctx.size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }

    qreal extraDamageMargin(const ToolContext &ctx) const override
    {
        return headLength(ctx);
    }

    void drawShape(QPainter &painter, const QPointF &anchor,
                   const QPointF &pos, const ToolContext &ctx) const override
    {
        if (anchor == pos) {
            painter.drawPoint(pos);
            return;
        }

        const QLineF line(anchor, pos);
        const qreal head = headLength(ctx);

        // Stop the shaft short of the tip so it does not poke through.
        QLineF shaft = line;
        shaft.setLength(qMax<qreal>(0, line.length() - head * 0.8));
        painter.drawLine(shaft);

        const qreal angle = std::atan2(-line.dy(), line.dx());
        const auto wing = [&](qreal offset) {
            return pos - QPointF(std::cos(angle + offset) * head,
                                 -std::sin(angle + offset) * head);
        };

        painter.save();
        painter.setBrush(ctx.color);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(QPolygonF() << pos << wing(-M_PI / 7) << wing(M_PI / 7));
        painter.restore();
    }

private:
    static qreal headLength(const ToolContext &ctx)
    {
        return qMax<qreal>(8.0, ctx.size * 4.0);
    }
};
