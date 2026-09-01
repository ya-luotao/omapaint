#include "ruby.h"

#include <QFile>
#include <QPainter>
#include <QRandomGenerator>
#include <QTextStream>

namespace Ruby {

namespace {

QStringList artRows()
{
    static QStringList rows = [] {
        QStringList result;
        QFile file(QStringLiteral(":/ruby.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd())
                result << in.readLine();
        }
        return result;
    }();
    return rows;
}

// One paw print: a pad with three toes, drawn around `center`. `size` is
// roughly the pad diameter in image pixels.
QRect stampPaw(QPainter &painter, const QPointF &center, qreal size)
{
    const qreal pad = size * 0.55;
    painter.drawEllipse(center + QPointF(0, size * 0.18), pad / 2, pad * 0.42);
    const qreal toe = size * 0.22;
    for (int i = -1; i <= 1; ++i)
        painter.drawEllipse(center
                                + QPointF(i * size * 0.30,
                                          -size * 0.32 + qAbs(i) * size * 0.10),
                            toe / 2, toe * 0.58);
    const qreal r = size;
    return QRectF(center.x() - r, center.y() - r, 2 * r, 2 * r).toAlignedRect();
}

} // namespace

QImage portrait(const QColor &fur, const QColor &accent, int cellSize)
{
    const QStringList rows = artRows();
    if (rows.isEmpty())
        return QImage();

    int width = 0;
    for (const QString &row : rows)
        width = qMax(width, int(row.size()));

    QImage cells(width, rows.size(), QImage::Format_ARGB32_Premultiplied);
    cells.fill(Qt::transparent);
    const QChar ink(0x2588), harness(u'R');
    for (int y = 0; y < rows.size(); ++y) {
        const QString &row = rows[y];
        for (int x = 0; x < row.size(); ++x) {
            if (row[x] == ink)
                cells.setPixelColor(x, y, fur);
            else if (row[x] == harness)
                cells.setPixelColor(x, y, accent);
        }
    }
    return cells.scaled(cells.size() * qMax(1, cellSize),
                        Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

QRect stampPawPrints(QImage &image, const QColor &color, quint32 seed)
{
    QRandomGenerator rng(seed);
    const qreal w = image.width(), h = image.height();
    // Paw size follows the canvas so the walk reads at any resolution.
    const qreal size = qBound(8.0, qMin(w, h) / 22.0, 64.0);

    // She wanders from the bottom-left third to the upper-right, weaving.
    QPointF pos(w * (0.05 + rng.bounded(0.15)), h * (0.75 + rng.bounded(0.15)));
    const QPointF goal(w * (0.80 + rng.bounded(0.15)),
                       h * (0.05 + rng.bounded(0.15)));
    const int steps = 7 + int(rng.bounded(3));

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    QRect damage;
    for (int i = 0; i < steps; ++i) {
        const qreal t = qreal(i) / (steps - 1);
        const QPointF along = pos + (goal - pos) * t;
        // Alternate paws left/right of the path, with a little wobble.
        const qreal side = (i % 2 == 0 ? -1 : 1) * size * 0.7;
        const QPointF step(along.x() + side + rng.bounded(int(size) / 2),
                           along.y() + rng.bounded(int(size) / 2));
        damage |= stampPaw(painter, step, size);
    }
    painter.end();
    return damage.intersected(image.rect());
}

} // namespace Ruby
