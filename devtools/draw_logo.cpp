// README-preview asset generator: draws the Omarchy icon and wordmark (as
// the About screen shows them) into a PNG using OmaPaint's own tool engine
// (RectangleTool outlines + FillTool floods), the way a user would.
//
// Usage: draw_logo <output.png> [background-color] [foreground-color]
// Colors are any QColor name ("#1f1f28", "white", ...). Defaults: white/black.
#include "tools/fill_tool.h"
#include "tools/rectangle_tool.h"

#include <QFile>
#include <QGuiApplication>
#include <QTextStream>

namespace {

QStringList readLines(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream in(&file);
    QStringList rows;
    while (!in.atEnd())
        rows << in.readLine();
    return rows;
}

// Renders one run of identical block characters as an outlined+filled rect.
void fillRect(QImage &image, const QPointF &tl, const QPointF &br,
              const ToolContext &ctx)
{
    RectangleTool rect;
    FillTool fill;
    rect.begin(image, tl, ctx);
    rect.move(image, br, ctx);
    rect.end(image, br, ctx);
    fill.begin(image, QPointF((tl.x() + br.x()) / 2, (tl.y() + br.y()) / 2), ctx);
}

// Handles █ (full cell), ▄ (lower half), ▀ (upper half).
void drawBlockArt(QImage &image, const QStringList &rows, qreal offX, qreal offY,
                  qreal cellW, qreal cellH, const ToolContext &ctx)
{
    const QChar full(0x2588), lower(0x2584), upper(0x2580);

    for (int y = 0; y < rows.size(); ++y) {
        const QString &row = rows[y];
        int x = 0;
        while (x < row.size()) {
            const QChar ch = row[x];
            if (ch != full && ch != lower && ch != upper) {
                ++x;
                continue;
            }
            int end = x;
            while (end + 1 < row.size() && row[end + 1] == ch)
                ++end;

            qreal top = offY + y * cellH;
            qreal bottom = top + cellH;
            if (ch == lower)
                top += cellH / 2;
            else if (ch == upper)
                bottom -= cellH / 2;

            fillRect(image,
                     QPointF(offX + x * cellW, top),
                     QPointF(offX + (end + 1) * cellW - 1, bottom - 1), ctx);

            x = end + 1;
        }
    }
}

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    if (argc < 2)
        return 2;

    QColor background(Qt::white);
    QColor foreground(Qt::black);
    if (argc > 2 && QColor::isValidColorName(QString::fromLocal8Bit(argv[2])))
        background = QColor::fromString(QString::fromLocal8Bit(argv[2]));
    if (argc > 3 && QColor::isValidColorName(QString::fromLocal8Bit(argv[3])))
        foreground = QColor::fromString(QString::fromLocal8Bit(argv[3]));

    QImage image(1280, 720, QImage::Format_ARGB32_Premultiplied);
    image.fill(background);

    const QStringList icon = readLines(QStringLiteral("/usr/share/omarchy/icon.txt"));
    const QStringList wordmark = readLines(QStringLiteral("/usr/share/omarchy/logo.txt"));
    if (icon.isEmpty() || wordmark.isEmpty())
        return 1;

    const ToolContext ctx{foreground, 1};

    // Icon: 54x26 half-width cells, centered in the upper area.
    const qreal iconW = 8, iconH = 16;
    drawBlockArt(image, icon, (1280 - 54 * iconW) / 2, 40, iconW, iconH, ctx);

    // Wordmark: 81x10 cells below, like the About screen.
    const qreal wordW = 10, wordH = 20;
    drawBlockArt(image, wordmark, (1280 - 81 * wordW) / 2, 40 + 26 * iconH + 40,
                 wordW, wordH, ctx);

    return image.save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
