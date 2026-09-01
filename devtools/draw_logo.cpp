// README-preview asset generator: draws the OmaPaint brand image — the
// hand-painted ring-and-dot logo, the "OmaPaint" wordmark, and the
// "Paint. For Omarchy." tagline — into a PNG using OmaPaint's own tool
// engine (BrushTool ring, EllipseTool dot, the text tool's renderer),
// the way a user would.
//
// Usage: draw_logo <output.png> [background-color] [foreground-color]
// Colors are any QColor name ("#1f1f28", "white", ...). Defaults: white/black.
// The paint dot stays OmaPaint red (#c34043) — it is the brand accent.
#include "text_renderer.h"
#include "tools/brush_tool.h"
#include "tools/ellipse_tool.h"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>

#include <cmath>

namespace {

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

// Centers one line of text horizontally and renders it via the same
// TextRenderer the text tool commits through.
void drawCenteredText(QImage &image, const QString &text, const QFont &font,
                      const QColor &color, qreal top)
{
    const QFontMetrics metrics(font);
    const qreal x = (image.width() - metrics.horizontalAdvance(text)) / 2.0;
    TextRenderer::render(image, QPointF(x, top), text, font, color);
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
    const QPointF center(640, 240);
    const qreal radius = 130;
    drawRing(image, center, radius, foreground, 42);
    drawDot(image, center + QPointF(88, 98), center + QPointF(172, 182));

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setBold(true);
    font.setPixelSize(110);
    drawCenteredText(image, QStringLiteral("OmaPaint"), font, foreground, 452);

    font.setBold(false);
    font.setPixelSize(40);
    drawCenteredText(image, QStringLiteral("Paint. For Omarchy."), font,
                     foreground, 610);

    return image.save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
