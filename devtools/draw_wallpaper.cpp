// Omarchy wallpaper generator: paints a minimal wallpaper in an Omarchy
// theme's colors using OmaPaint's own tool engine — the Omarchy block-art
// icon in the theme's muted ink, with the theme's terminal palette as a
// strip of solid squares below it.
//
// Usage: draw_wallpaper <output.png> <colors.toml> [WxH]
// Default size is 3840x2400; the layout scales with the smaller dimension.
// Exits 3 when colors.toml lacks background/foreground (half-applying a
// theme is worse than none — same rule as the app's Theme loader).
//
// The block-art helpers are duplicated from draw_logo.cpp on purpose:
// devtools stay self-contained rather than growing shared headers.
#include "tools/fill_tool.h"
#include "tools/rectangle_tool.h"

#include <QFile>
#include <QGuiApplication>
#include <QHash>
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

// Minimal colors.toml reader: `key = "value"` lines only.
QHash<QString, QString> readTheme(const QString &path)
{
    QHash<QString, QString> values;
    for (const QString &line : readLines(path)) {
        const qsizetype eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.first(eq).trimmed();
        QString value = line.sliced(eq + 1).trimmed();
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"')))
            value = value.mid(1, value.indexOf(QLatin1Char('"'), 1) - 1);
        values.insert(key, value);
    }
    return values;
}

QColor themeColor(const QHash<QString, QString> &theme, const QStringList &keys)
{
    for (const QString &key : keys)
        if (QColor::isValidColorName(theme.value(key)))
            return QColor::fromString(theme.value(key));
    return {};
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

void solidSquare(QImage &image, const QPointF &tl, qreal side, const QColor &color)
{
    RectangleTool rect;
    const ToolContext ctx{color, 1, color, ShapeFill::Solid};
    rect.begin(image, tl, ctx);
    rect.move(image, QPointF(tl.x() + side - 1, tl.y() + side - 1), ctx);
    rect.end(image, QPointF(tl.x() + side - 1, tl.y() + side - 1), ctx);
}

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    if (argc < 3) {
        fprintf(stderr, "usage: draw_wallpaper <output.png> <colors.toml> [WxH]\n");
        return 2;
    }

    int width = 3840, height = 2400;
    if (argc > 3)
        sscanf(argv[3], "%dx%d", &width, &height);
    if (width < 640 || height < 400)
        return 2;

    const QHash<QString, QString> theme = readTheme(QString::fromLocal8Bit(argv[2]));
    const QColor background = themeColor(theme, {"background"});
    const QColor foreground = themeColor(theme, {"foreground"});
    if (!background.isValid() || !foreground.isValid())
        return 3; // incomplete theme: skip rather than half-apply

    // Subtle ink for the icon: prefer the theme's muted tone over full
    // foreground so the wallpaper stays a backdrop, not a poster.
    const bool light = theme.value("mode") == QLatin1String("light");
    const QColor ink = light
        ? themeColor(theme, {"muted", "light_foreground", "foreground"})
        : themeColor(theme, {"muted", "dark_foreground", "foreground"});

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(background);

    const QStringList icon = readLines(QStringLiteral("/usr/share/omarchy/icon.txt"));
    if (icon.isEmpty())
        return 1;

    // Layout in 3840x2400 reference units, scaled to the smaller dimension.
    const qreal s = qMin(width / 3840.0, height / 2400.0);
    const qreal cellW = 14 * s, cellH = 28 * s;
    const qreal iconW = 54 * cellW, iconH = 26 * cellH;
    const qreal iconX = (width - iconW) / 2;
    const qreal iconY = (height - iconH) / 2 - 120 * s;
    drawBlockArt(image, icon, iconX, iconY, cellW, cellH, {ink, 1});

    // Terminal palette strip, centered under the icon.
    QList<QColor> palette;
    for (const char *key : {"red", "orange", "yellow", "green", "cyan", "blue", "magenta"}) {
        const QColor c = themeColor(theme, {QString::fromLatin1(key)});
        if (c.isValid() && c != background)
            palette << c;
    }
    if (!palette.isEmpty()) {
        const qreal side = 56 * s, gap = 28 * s;
        const qreal stripW = palette.size() * side + (palette.size() - 1) * gap;
        qreal x = (width - stripW) / 2;
        const qreal y = iconY + iconH + 140 * s;
        for (const QColor &c : palette) {
            solidSquare(image, QPointF(x, y), side, c);
            x += side + gap;
        }
    }

    return image.save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
