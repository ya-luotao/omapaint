#pragma once

#include "tool.h"

// Erases to transparency so alpha-capable formats keep their alpha channel.
class EraserTool : public StrokeTool
{
public:
    QString name() const override { return QStringLiteral("Eraser"); }

protected:
    QPen pen(const ToolContext &ctx) const override
    {
        return QPen(Qt::transparent, ctx.size, Qt::SolidLine, Qt::RoundCap,
                    Qt::RoundJoin);
    }

    QPainter::CompositionMode compositionMode() const override
    {
        return QPainter::CompositionMode_Clear;
    }
};
