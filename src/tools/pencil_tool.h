#pragma once

#include "tool.h"

// Always 1px, regardless of the brush size setting.
class PencilTool : public StrokeTool
{
public:
    QString name() const override { return QStringLiteral("Pencil"); }

protected:
    QPen pen(const ToolContext &ctx) const override
    {
        return QPen(ctx.color, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    }
};
