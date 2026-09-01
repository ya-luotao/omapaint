#include "pixelate_tool.h"

namespace {
constexpr int kBlockSize = 10;
} // namespace

QRect PixelateTool::begin(QImage &image, const QPointF &pos, const ToolContext &)
{
    m_anchor = pos;
    m_snapshot = image; // copy-on-write
    m_previewDamage = QRect();
    return drawPreview(image, pos);
}

QRect PixelateTool::move(QImage &image, const QPointF &pos, const ToolContext &)
{
    const QRect restored = restorePreview(image);
    return restored | drawPreview(image, pos);
}

QRect PixelateTool::end(QImage &image, const QPointF &pos, const ToolContext &)
{
    QRect damage = restorePreview(image);

    const QRect rect = QRectF(m_anchor, pos)
                           .normalized()
                           .toAlignedRect()
                           .intersected(image.rect());
    if (rect.width() >= 2 && rect.height() >= 2) {
        const QImage region = image.copy(rect);
        const QSize coarse(qMax(1, rect.width() / kBlockSize),
                           qMax(1, rect.height() / kBlockSize));
        const QImage blocks =
            region.scaled(coarse, Qt::IgnoreAspectRatio, Qt::FastTransformation)
                .scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);

        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawImage(rect.topLeft(), blocks);
        painter.end();
        damage |= rect;
    }

    m_snapshot = QImage();
    m_previewDamage = QRect();
    return damage;
}

QRect PixelateTool::drawPreview(QImage &image, const QPointF &pos)
{
    const QRectF rect = QRectF(m_anchor, pos).normalized();

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(Qt::black, 1, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
    painter.end();

    m_previewDamage = rect.adjusted(-2, -2, 2, 2)
                          .toAlignedRect()
                          .intersected(image.rect());
    return m_previewDamage;
}

QRect PixelateTool::restorePreview(QImage &image)
{
    if (m_previewDamage.isEmpty())
        return QRect();

    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(m_previewDamage.topLeft(), m_snapshot, m_previewDamage);
    painter.end();

    const QRect restored = m_previewDamage;
    m_previewDamage = QRect();
    return restored;
}
