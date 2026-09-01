#include "tool.h"

#include <QtMath>

namespace {

QRect penAdjustedRect(const QRectF &bounds, const QPen &pen, const QImage &image)
{
    const int margin = qCeil(pen.widthF() / 2.0) + 1;
    return bounds.normalized()
        .adjusted(-margin, -margin, margin, margin)
        .toAlignedRect()
        .intersected(image.rect());
}

} // namespace

QRect Tool::end(QImage &, const QPointF &, const ToolContext &)
{
    return QRect();
}

// --- StrokeTool ---

QRect StrokeTool::begin(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    m_last = pos;
    return drawSegment(image, pos, pos, ctx);
}

QRect StrokeTool::move(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    const QRect damage = drawSegment(image, m_last, pos, ctx);
    m_last = pos;
    return damage;
}

QRect StrokeTool::drawSegment(QImage &image, const QPointF &from, const QPointF &to,
                              const ToolContext &ctx) const
{
    const QPen p = pen(ctx);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setCompositionMode(compositionMode());
    painter.setPen(p);
    // A zero-length line is not stroked for wide pens; draw a dot instead.
    if (from == to)
        painter.drawPoint(from);
    else
        painter.drawLine(from, to);
    painter.end();

    return penAdjustedRect(QRectF(from, to), p, image);
}

// --- ShapeTool ---

QPen ShapeTool::pen(const ToolContext &ctx) const
{
    return QPen(ctx.color, ctx.size, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
}

QRect ShapeTool::begin(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    m_anchor = pos;
    m_snapshot = image; // copy-on-write snapshot
    m_previewDamage = QRect();
    return paintPreview(image, pos, ctx);
}

QRect ShapeTool::move(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    return paintPreview(image, pos, ctx);
}

QRect ShapeTool::end(QImage &, const QPointF &, const ToolContext &)
{
    m_snapshot = QImage();
    m_previewDamage = QRect();
    return QRect();
}

QRect ShapeTool::paintPreview(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    QRect damage;

    // Restore what the previous preview painted over.
    if (!m_previewDamage.isEmpty()) {
        QPainter restore(&image);
        restore.setCompositionMode(QPainter::CompositionMode_Source);
        restore.drawImage(m_previewDamage.topLeft(), m_snapshot, m_previewDamage);
        restore.end();
        damage = m_previewDamage;
    }

    const QPen p = pen(ctx);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(p);
    painter.setBrush(Qt::NoBrush);
    drawShape(painter, m_anchor, pos);
    painter.end();

    m_previewDamage = penAdjustedRect(QRectF(m_anchor, pos), p, image);
    return damage | m_previewDamage;
}
