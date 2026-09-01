// Dogfood: every tool icon and the application logo are drawn by OmaPaint's
// own tool engine. Regenerate with:
//   cmake --build build --target draw_icons && ./build/draw_icons
// The PNGs are committed to assets/ and embedded via the Qt resource system,
// so this tool is only needed when the icons change.
#include "text_renderer.h"
#include "tools/arrow_tool.h"
#include "tools/brush_tool.h"
#include "tools/ellipse_tool.h"
#include "tools/fill_tool.h"
#include "tools/line_tool.h"
#include "tools/pencil_tool.h"
#include "tools/rectangle_tool.h"

#include <QDir>
#include <QGuiApplication>

namespace {

constexpr int kIconSize = 128;

ToolContext ink(qreal size, ShapeFill fill = ShapeFill::Outline)
{
    ToolContext ctx;
    ctx.color = Qt::black; // tinted to the theme's text color at runtime
    ctx.fillColor = Qt::black;
    ctx.size = size;
    ctx.shapeFill = fill;
    return ctx;
}

void stroke(Tool &tool, QImage &image, const ToolContext &ctx,
            std::initializer_list<QPointF> points)
{
    auto it = points.begin();
    tool.begin(image, *it, ctx);
    for (++it; it != points.end(); ++it)
        tool.move(image, *it, ctx);
    tool.end(image, *(points.end() - 1), ctx);
}

QImage blank()
{
    QImage image(kIconSize, kIconSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

void drawSelect(QImage &image)
{
    LineTool line;
    const ToolContext ctx = ink(7);
    // Dashed marquee rectangle from short line segments.
    const qreal lo = 22, hi = 106;
    for (qreal t = lo; t < hi; t += 28) {
        stroke(line, image, ctx, {QPointF(t, lo), QPointF(t + 16, lo)});
        stroke(line, image, ctx, {QPointF(t, hi), QPointF(t + 16, hi)});
        stroke(line, image, ctx, {QPointF(lo, t), QPointF(lo, t + 16)});
        stroke(line, image, ctx, {QPointF(hi, t), QPointF(hi, t + 16)});
    }
}

void drawPencil(QImage &image)
{
    BrushTool brush;
    stroke(brush, image, ink(6),
           {QPointF(20, 100), QPointF(45, 60), QPointF(70, 90),
            QPointF(95, 35), QPointF(108, 48)});
}

void drawBrush(QImage &image)
{
    BrushTool brush;
    stroke(brush, image, ink(20),
           {QPointF(24, 100), QPointF(50, 55), QPointF(80, 75),
            QPointF(104, 28)});
}

void drawEraser(QImage &image)
{
    RectangleTool rect;
    ToolContext body = ink(6, ShapeFill::Solid);
    stroke(rect, image, body, {QPointF(40, 32), QPointF(92, 72)});
    BrushTool smear;
    stroke(smear, image, ink(10), {QPointF(24, 98), QPointF(104, 98)});
}

void drawLine(QImage &image)
{
    LineTool line;
    stroke(line, image, ink(9), {QPointF(24, 104), QPointF(104, 24)});
}

void drawArrow(QImage &image)
{
    ArrowTool arrow;
    stroke(arrow, image, ink(9), {QPointF(24, 104), QPointF(104, 24)});
}

void drawRectangle(QImage &image)
{
    RectangleTool rect;
    stroke(rect, image, ink(9), {QPointF(24, 34), QPointF(104, 94)});
}

void drawEllipse(QImage &image)
{
    EllipseTool ellipse;
    stroke(ellipse, image, ink(9), {QPointF(22, 30), QPointF(106, 98)});
}

void drawFill(QImage &image)
{
    // Pouring stroke above a puddle.
    BrushTool pour;
    stroke(pour, image, ink(13), {QPointF(84, 22), QPointF(56, 62)});
    EllipseTool puddle;
    stroke(puddle, image, ink(4, ShapeFill::Solid),
           {QPointF(26, 84), QPointF(102, 112)});
}

void drawText(QImage &image)
{
    QFont font;
    font.setPixelSize(104);
    font.setBold(true);
    TextRenderer::render(image, QPointF(30, 6), QStringLiteral("A"), font,
                         Qt::black);
}

void drawPixelate(QImage &image)
{
    RectangleTool rect;
    const ToolContext solid = ink(3, ShapeFill::Solid);
    const ToolContext outline = ink(3);
    const qreal s = 28;
    stroke(rect, image, solid, {QPointF(22, 22), QPointF(22 + s, 22 + s)});
    stroke(rect, image, solid, {QPointF(50, 50), QPointF(50 + s, 50 + s)});
    stroke(rect, image, solid, {QPointF(78, 78), QPointF(78 + s, 78 + s)});
    stroke(rect, image, outline, {QPointF(78, 22), QPointF(78 + s, 22 + s)});
    stroke(rect, image, outline, {QPointF(22, 78), QPointF(22 + s, 78 + s)});
}

void drawEyedropper(QImage &image)
{
    BrushTool stem;
    stroke(stem, image, ink(10), {QPointF(102, 26), QPointF(52, 76)});
    EllipseTool drop;
    stroke(drop, image, ink(4, ShapeFill::Solid),
           {QPointF(32, 72), QPointF(60, 100)});
}

// 256x256 application logo: a hand-painted "O" plus a paint dot, in fixed
// Omarchy-flavored colors (the logo should not change with the theme).
QImage drawLogo()
{
    QImage image(256, 256, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    RectangleTool bg;
    ToolContext dark;
    dark.color = QColor("#1f1f28");
    dark.fillColor = dark.color;
    dark.size = 1;
    dark.shapeFill = ShapeFill::Solid;
    stroke(bg, image, dark, {QPointF(0, 0), QPointF(255, 255)});

    // Brush-drawn circle: segments around the ring so it looks hand-painted.
    BrushTool brush;
    ToolContext beige;
    beige.color = QColor("#dcd7ba");
    beige.size = 24;
    const QPointF center(128, 122);
    const qreal radius = 74;
    brush.begin(image, center + QPointF(radius, 0), beige);
    for (int i = 1; i <= 28; ++i) {
        const qreal angle = i * 2.0 * M_PI / 28;
        brush.move(image,
                   center + QPointF(std::cos(angle) * radius,
                                    std::sin(angle) * radius),
                   beige);
    }
    brush.end(image, center + QPointF(radius, 0), beige);

    // The paint dot is named Ruby, after a chubby white cat with a
    // ruby-red harness. She also stars in the demo (src/demo_driver.cpp).
    EllipseTool dot;
    ToolContext red;
    red.color = QColor("#c34043");
    red.fillColor = red.color;
    red.size = 1;
    red.shapeFill = ShapeFill::Solid;
    stroke(dot, image, red, {QPointF(178, 178), QPointF(226, 226)});

    return image;
}

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : QStringLiteral("assets");
    QDir().mkpath(outDir + QStringLiteral("/icons"));

    const struct {
        const char *name;
        void (*draw)(QImage &);
    } icons[] = {
        {"select", drawSelect},       {"pencil", drawPencil},
        {"brush", drawBrush},         {"eraser", drawEraser},
        {"line", drawLine},           {"arrow", drawArrow},
        {"rectangle", drawRectangle}, {"ellipse", drawEllipse},
        {"fill", drawFill},           {"text", drawText},
        {"pixelate", drawPixelate},   {"eyedropper", drawEyedropper},
    };

    for (const auto &icon : icons) {
        QImage image = blank();
        icon.draw(image);
        const QString path =
            outDir + QStringLiteral("/icons/%1.png").arg(QLatin1String(icon.name));
        if (!image.save(path))
            return 1;
    }

    if (!drawLogo().save(outDir + QStringLiteral("/omapaint.png")))
        return 1;

    return 0;
}
