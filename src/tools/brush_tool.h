#pragma once

#include "tool.h"

class BrushTool : public StrokeTool
{
public:
    QString name() const override { return QStringLiteral("Brush"); }

protected:
    QPen pen(const ToolContext &ctx) const override
    {
        return QPen(ctx.color, ctx.size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }
};
