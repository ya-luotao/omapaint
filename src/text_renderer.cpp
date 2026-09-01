#include "text_renderer.h"

#include <QPainter>

namespace TextRenderer {

QRect render(QImage &image, const QPointF &pos, const QString &text,
             const QFont &font, const QColor &color)
{
    if (text.trimmed().isEmpty())
        return QRect();

    // Unbounded layout rect: newlines break lines, nothing auto-wraps.
    const QRectF layout(pos, QSizeF(1e6, 1e6));
    const int flags = Qt::AlignLeft | Qt::AlignTop;

    QPainter painter(&image);
    painter.setFont(font);
    painter.setPen(color);
    // Damage from the same API that renders, so it matches the actual ink;
    // padded because italics and some glyphs overhang the layout box.
    const QRectF bounds = painter.boundingRect(layout, flags, text);
    painter.drawText(layout, flags, text);
    painter.end();

    return bounds.toAlignedRect().adjusted(-2, -2, 2, 2).intersected(image.rect());
}

} // namespace TextRenderer
