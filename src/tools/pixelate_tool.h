#pragma once

#include "tool.h"

// Drag out a rectangle; on release its content is mosaicked — the everyday
// way to hide sensitive parts of a screenshot. Fixed block size so the
// hiding granularity does not silently follow the brush-size setting.
class PixelateTool : public Tool
{
public:
    QString name() const override { return QStringLiteral("Pixelate"); }

    QRect begin(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect move(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect end(QImage &image, const QPointF &pos, const ToolContext &ctx) override;

private:
    QRect drawPreview(QImage &image, const QPointF &pos);
    QRect restorePreview(QImage &image);

    QPointF m_anchor;
    QImage m_snapshot;
    QRect m_previewDamage;
};
