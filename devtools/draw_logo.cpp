// README-preview asset generator: draws the OmaPaint brand image — the
// hand-painted ring-and-dot logo, the OMAPAINT wordmark in the Omarchy
// logo's own pixel style, and the "Paint. For Omarchy." tagline — into a
// PNG using OmaPaint's own tool engine, the way a user would.
//
// Usage: draw_logo <output.png> [background-color] [foreground-color]
// Colors are any QColor name ("#1f1f28", "white", ...). Defaults: white/black.
// The paint dot stays ruby red (#c34043) — it is the brand accent, and it
// has a name: Ruby, after a chubby white cat with a ruby-red harness.
#include "text_renderer.h"
#include "tools/brush_tool.h"
#include "tools/ellipse_tool.h"
#include "tools/rectangle_tool.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>

#include <cmath>

namespace {

// OMAPAINT in the Omarchy wordmark's glyph style: O, M, A are the original
// letters (/usr/share/omarchy/logo.txt); P, I, N, T were drawn to match
// (3-cell strokes, half-block corners, tapered stem ends).
const QString kWordmark[] = {
    QStringLiteral("                 ▄▄▄"),
    QStringLiteral(" ▄█████▄    ▄███████████▄     ▄███████    ▄███████    ▄███████  ▄█▄   ▄█████▄   █████████"),
    QStringLiteral("███   ███  ███   ███   ███   ███   ███   ███   ███   ███   ███  ███  ███   ███  ▀▀▀███▀▀▀"),
    QStringLiteral("███   ███  ███   ███   ███   ███   ███   ███   ███   ███   ███  ███  ███   ███     ███"),
    QStringLiteral("███   ███  ███   ███   ███  ▄███▄▄▄███  ▄███▄▄▄██▀  ▄███▄▄▄███  ███  ███   ███     ███"),
    QStringLiteral("███   ███  ███   ███   ███  ▀███▀▀▀███  ▀███▀▀▀▀    ▀███▀▀▀███  ███  ███   ███     ███"),
    QStringLiteral("███   ███  ███   ███   ███   ███   ███   ███         ███   ███  ███  ███   ███     ███"),
    QStringLiteral("███   ███  ███   ███   ███   ███   ███   ███         ███   ███  ███  ███   ███     ███"),
    QStringLiteral(" ▀█████▀    ▀█   ███   █▀    ███   █▀    ███         ███   █▀   ▀█▀   ▀█   █▀      ▀█▀")
};

// Brush-drawn circle: segments around the ring so it looks hand-painted,
// same construction as the committed application logo (devtools/draw_icons).
void drawRing(QImage &image, const QPointF &center, qreal radius,
              const QColor &color, qreal brushSize)
{
    BrushTool brush;
    ToolContext ctx;
    ctx.color = color;
    ctx.size = brushSize;
    brush.begin(image, center + QPointF(radius, 0), ctx);
    for (int i = 1; i <= 28; ++i) {
        const qreal angle = i * 2.0 * M_PI / 28;
        brush.move(image,
                   center + QPointF(std::cos(angle) * radius,
                                    std::sin(angle) * radius),
                   ctx);
    }
    brush.end(image, center + QPointF(radius, 0), ctx);
}

void drawDot(QImage &image, const QPointF &tl, const QPointF &br)
{
    EllipseTool dot;
    ToolContext red;
    red.color = QColor("#c34043");
    red.fillColor = red.color;
    red.size = 1;
    red.shapeFill = ShapeFill::Solid;
    dot.begin(image, tl, red);
    dot.move(image, br, red);
    dot.end(image, br, red);
}

// Paints one run of identical block characters as a solid rectangle.
void fillCells(QImage &image, const QPointF &tl, const QPointF &br,
               const QColor &color)
{
    RectangleTool rect;
    ToolContext ctx;
    ctx.color = color;
    ctx.fillColor = color;
    ctx.size = 1;
    ctx.shapeFill = ShapeFill::Solid;
    rect.begin(image, tl, ctx);
    rect.move(image, br, ctx);
    rect.end(image, br, ctx);
}

// Handles █ (full cell), ▄ (lower half), ▀ (upper half).
void drawBlockArt(QImage &image, const QString *rows, int rowCount, qreal offX,
                  qreal offY, qreal cellW, qreal cellH, const QColor &color)
{
    const QChar full(0x2588), lower(0x2584), upper(0x2580);

    for (int y = 0; y < rowCount; ++y) {
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

            fillCells(image, QPointF(offX + x * cellW, top),
                      QPointF(offX + (end + 1) * cellW - 1, bottom - 1), color);

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

    // Ring and dot: the application logo's proportions (center offset dot at
    // the lower right), scaled up from its 256px master.
    const QPointF center(640, 200);
    const qreal radius = 105;
    drawRing(image, center, radius, foreground, 36);
    drawDot(image, center + QPointF(71, 79), center + QPointF(139, 147));

    // Wordmark: 89x10 cells, centered below the ring.
    constexpr int kRows = int(sizeof(kWordmark) / sizeof(kWordmark[0]));
    const qreal cellW = 11, cellH = 22;
    drawBlockArt(image, kWordmark, kRows, (1280 - 89 * cellW) / 2, 375, cellW,
                 cellH, foreground);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(36);
    const QString tagline = QStringLiteral("Paint. For Omarchy.");
    const QFontMetrics metrics(font);
    TextRenderer::render(
        image, QPointF((1280 - metrics.horizontalAdvance(tagline)) / 2.0, 640),
        tagline, font, foreground);

    return image.save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
