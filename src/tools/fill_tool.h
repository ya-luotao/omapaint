#pragma once

#include "tool.h"

// Flood fill (exact color match, 4-connected) applied on pointer press.
class FillTool : public Tool
{
public:
    QString name() const override { return QStringLiteral("Fill"); }

    QRect begin(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect move(QImage &, const QPointF &, const ToolContext &) override
    {
        return QRect();
    }
};
