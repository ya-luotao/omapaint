#include "fill_tool.h"

#include <QStack>

QRect FillTool::begin(QImage &image, const QPointF &pos, const ToolContext &ctx)
{
    const QPoint seed = pos.toPoint();
    if (!image.rect().contains(seed))
        return QRect();

    const QRgb target = image.pixel(seed);
    const QRgb fill = qPremultiply(ctx.color.rgba());
    if (target == fill)
        return QRect();

    // Scanline flood fill: pop a seed, expand its horizontal span, then queue
    // spans above and below.
    const int w = image.width();
    const int h = image.height();
    int minX = seed.x(), maxX = seed.x(), minY = seed.y(), maxY = seed.y();

    QStack<QPoint> stack;
    stack.push(seed);

    while (!stack.isEmpty()) {
        const QPoint p = stack.pop();
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(p.y()));
        if (line[p.x()] != target)
            continue;

        int left = p.x();
        while (left > 0 && line[left - 1] == target)
            --left;
        int right = p.x();
        while (right < w - 1 && line[right + 1] == target)
            ++right;

        for (int x = left; x <= right; ++x)
            line[x] = fill;

        minX = qMin(minX, left);
        maxX = qMax(maxX, right);
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());

        for (int dy : {-1, 1}) {
            const int y = p.y() + dy;
            if (y < 0 || y >= h)
                continue;
            const QRgb *neighbor = reinterpret_cast<const QRgb *>(image.scanLine(y));
            for (int x = left; x <= right; ++x) {
                if (neighbor[x] == target) {
                    stack.push(QPoint(x, y));
                    // Skip the rest of this run; the popped seed expands it.
                    while (x <= right && neighbor[x] == target)
                        ++x;
                }
            }
        }
    }

    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}
