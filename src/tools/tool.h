#pragma once

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRect>
#include <QString>

// Settings the canvas passes to the active tool for each stroke.
struct ToolContext
{
    QColor color = Qt::black;
    qreal size = 1;
};

// A tool paints directly into the document image while the pointer is down
// and reports the damaged rect of each step. The canvas accumulates the
// damage and commits one DrawCommand per stroke on release.
class Tool
{
public:
    virtual ~Tool() = default;

    virtual QString name() const = 0;

    virtual QRect begin(QImage &image, const QPointF &pos, const ToolContext &ctx) = 0;
    virtual QRect move(QImage &image, const QPointF &pos, const ToolContext &ctx) = 0;
    virtual QRect end(QImage &image, const QPointF &pos, const ToolContext &ctx);
};

// Freehand tools: pencil, brush, eraser. Paints a segment per pointer move.
class StrokeTool : public Tool
{
public:
    QRect begin(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect move(QImage &image, const QPointF &pos, const ToolContext &ctx) override;

protected:
    virtual QPen pen(const ToolContext &ctx) const = 0;
    virtual QPainter::CompositionMode compositionMode() const
    {
        return QPainter::CompositionMode_SourceOver;
    }

private:
    QRect drawSegment(QImage &image, const QPointF &from, const QPointF &to,
                      const ToolContext &ctx) const;

    QPointF m_last;
};

// Rubber-band tools: line, rectangle, ellipse. Each pointer move restores
// the previously previewed region from a snapshot (cheap: QImage is
// copy-on-write) and repaints the shape from the anchor to the current
// position, so the final image is identical to drawing the shape once.
class ShapeTool : public Tool
{
public:
    QRect begin(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect move(QImage &image, const QPointF &pos, const ToolContext &ctx) override;
    QRect end(QImage &image, const QPointF &pos, const ToolContext &ctx) override;

protected:
    virtual void drawShape(QPainter &painter, const QPointF &anchor,
                           const QPointF &pos) const = 0;
    virtual QPen pen(const ToolContext &ctx) const;

private:
    QRect paintPreview(QImage &image, const QPointF &pos, const ToolContext &ctx);

    QPointF m_anchor;
    QImage m_snapshot;
    QRect m_previewDamage;
};
