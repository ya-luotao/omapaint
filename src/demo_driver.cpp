#include "demo_driver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMouseEvent>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

namespace {

// Same block-art geometry the README preview uses.
constexpr qreal kIconCellW = 8, kIconCellH = 16;
constexpr qreal kWordCellW = 10, kWordCellH = 20;
constexpr qreal kIconOffX = (1280 - 54 * kIconCellW) / 2.0;
constexpr qreal kIconOffY = 20;
constexpr qreal kWordOffY = kIconOffY + 26 * kIconCellH + 40;
constexpr qreal kWordOffX = (1280 - 81 * kWordCellW) / 2.0;
// Ruby (assets/ruby.txt, 38x32 cells) sits in the empty top-right corner.
constexpr qreal kRubyCell = 7;
constexpr qreal kRubyOffX = 1250 - 38 * kRubyCell;
constexpr qreal kRubyOffY = 56;

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

// The demo paints in the active Omarchy theme's colors, like the README
// preview: theme background flooded onto the canvas, foreground as ink.
QColor themeColor(const char *key, const QColor &fallback)
{
    QFile file(QDir::homePath()
               + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fallback;
    const QRegularExpression line(
        QStringLiteral("^%1\\s*=\\s*\"(#[0-9a-fA-F]{6,8})\"").arg(QLatin1String(key)));
    QTextStream in(&file);
    while (!in.atEnd()) {
        const auto match = line.match(in.readLine());
        if (match.hasMatch())
            return QColor(match.captured(1));
    }
    return fallback;
}

} // namespace

DemoDriver::DemoDriver(QQuickWindow *window, CanvasItem *canvas, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_canvas(canvas)
{
    QFile themeName(QDir::homePath()
                    + QStringLiteral("/.local/state/omarchy/current/theme.name"));
    if (themeName.open(QIODevice::ReadOnly))
        m_originalTheme = QString::fromUtf8(themeName.readAll()).trimmed();

    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        while (m_next < m_steps.size()
               && m_steps[m_next].time <= m_clock.elapsed()) {
            m_steps[m_next].fn();
            ++m_next;
        }
        if (m_next >= m_steps.size()) {
            m_timer.stop();
            emit finished();
        }
    });
}

void DemoDriver::start()
{
    buildScript();
    m_clock.start();
    m_timer.start();
}

// --- timeline building ---

void DemoDriver::at(std::function<void()> fn)
{
    m_steps.append({m_cursor, std::move(fn)});
}

QPointF DemoDriver::scenePos(const QPointF &imagePos) const
{
    const QPointF itemPos = m_canvas->viewOrigin() + imagePos * m_canvas->zoom();
    return m_canvas->mapToScene(itemPos);
}

void DemoDriver::postMouse(QEvent::Type type, const QPointF &imagePos,
                           Qt::MouseButton button, Qt::MouseButtons buttons)
{
    const QPointF scene = scenePos(imagePos);
    QCoreApplication::postEvent(
        m_window,
        new QMouseEvent(type, scene,
                        m_window->mapToGlobal(scene.toPoint()), button,
                        buttons, Qt::NoModifier));
}

void DemoDriver::press(const QPointF &imagePos)
{
    at([this, imagePos] {
        postMouse(QEvent::MouseButtonPress, imagePos, Qt::LeftButton,
                  Qt::LeftButton);
    });
}

void DemoDriver::release(const QPointF &imagePos)
{
    at([this, imagePos] {
        postMouse(QEvent::MouseButtonRelease, imagePos, Qt::LeftButton,
                  Qt::NoButton);
    });
}

void DemoDriver::moveTo(const QPointF &from, const QPointF &to, int duration)
{
    const int steps = qMax(2, duration / 16);
    for (int i = 1; i <= steps; ++i) {
        const QPointF pos = from + (to - from) * (qreal(i) / steps);
        m_cursor += duration / steps;
        at([this, pos] {
            // Mid-drag moves must carry the pressed-button state.
            postMouse(QEvent::MouseMove, pos, Qt::NoButton, Qt::LeftButton);
        });
    }
}

void DemoDriver::dragTo(const QPointF &from, const QPointF &to, int duration)
{
    press(from);
    wait(16);
    moveTo(from, to, duration);
    wait(16);
    release(to);
}

void DemoDriver::click(const QPointF &imagePos)
{
    press(imagePos);
    wait(30);
    release(imagePos);
}

void DemoDriver::typeText(const QString &text, int msPerChar)
{
    for (const QChar c : text) {
        at([this, c] {
            QCoreApplication::postEvent(
                m_window, new QKeyEvent(QEvent::KeyPress, 0, Qt::NoModifier,
                                        QString(c)));
            QCoreApplication::postEvent(
                m_window, new QKeyEvent(QEvent::KeyRelease, 0, Qt::NoModifier));
        });
        wait(msPerChar);
    }
}

void DemoDriver::setTool(CanvasItem::ToolType tool)
{
    at([this, tool] { m_canvas->setTool(tool); });
}

void DemoDriver::setColor(const QColor &color)
{
    at([this, color] { m_canvas->setForegroundColor(color); });
}

void DemoDriver::setFillMode(CanvasItem::ShapeFillMode mode)
{
    at([this, mode] { m_canvas->setShapeFillMode(mode); });
}

void DemoDriver::setBrushSize(int size)
{
    at([this, size] { m_canvas->setBrushSize(size); });
}

void DemoDriver::setTheme(const QString &name)
{
    at([name] {
        QProcess::startDetached(QStringLiteral("omarchy-theme-set"), {name});
    });
}

// Speed-paints one block-art file with solid rectangles.
void DemoDriver::buildBlockArt(const QString &path, qreal offX, qreal offY,
                               qreal cellW, qreal cellH, int msPerBlock,
                               QChar cell)
{
    const QChar full(cell), lower(0x2584), upper(0x2580);
    const QStringList rows = readLines(path);

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

            dragTo(QPointF(offX + x * cellW, top),
                   QPointF(offX + (end + 1) * cellW - 1, bottom - 1),
                   qMax(40, msPerBlock - 30));
            wait(12);

            x = end + 1;
        }
    }
}

void DemoDriver::buildScript()
{
    const QColor bg = themeColor("background", Qt::white);
    const QColor kInk = themeColor("foreground", Qt::black);
    const QColor kAccent = themeColor("red", QColor("#c34043"));

    // 0. Flood the canvas with the theme background — Fill tool as opener.
    wait(350);
    setTool(CanvasItem::Fill);
    setColor(bg);
    click(QPointF(640, 360));
    wait(300);

    // 1. Speed-paint the Omarchy mark with solid rectangles.
    setTool(CanvasItem::Rectangle);
    setFillMode(CanvasItem::Solid);
    setColor(kInk);
    setBrushSize(2);
    wait(100);
    buildBlockArt(QStringLiteral("/usr/share/omarchy/icon.txt"), kIconOffX,
                  kIconOffY, kIconCellW, kIconCellH, 60);
    wait(180);
    buildBlockArt(QStringLiteral("/usr/share/omarchy/logo.txt"), kWordOffX,
                  kWordOffY, kWordCellW, kWordCellH, 55);
    wait(220);

    // 2. A red period after the wordmark.
    setTool(CanvasItem::Ellipse);
    setColor(kAccent);
    dragTo(QPointF(1052, 638), QPointF(1098, 684), 200);
    wait(350);

    // 3. Text tool: type the tagline into the live overlay.
    setTool(CanvasItem::Text);
    setColor(kInk);
    at([this] {
        QFont font;
        font.setPixelSize(34);
        m_canvas->setTextFont(font);
    });
    click(QPointF(30, 320));
    wait(250);
    typeText(QStringLiteral("Paint. For Omarchy."), 60);
    wait(350);
    click(QPointF(200, 620)); // commit by clicking elsewhere
    wait(300);

    // 4. Red arrow from the tagline to the mark.
    setTool(CanvasItem::Arrow);
    setColor(kAccent);
    setBrushSize(7);
    dragTo(QPointF(210, 300), QPointF(400, 200), 380);
    wait(400);

    // 5. Speed-paint Ruby — the cat the red dot is named after — as block
    // art in the empty top-right corner: same visual language as the mark,
    // theme foreground as fur, theme red as her harness and her period.
    setTool(CanvasItem::Rectangle);
    setFillMode(CanvasItem::Solid);
    setColor(kInk);
    buildBlockArt(QStringLiteral(":/ruby.txt"), kRubyOffX, kRubyOffY,
                  kRubyCell, kRubyCell, 55);
    wait(200);
    setColor(kAccent);
    buildBlockArt(QStringLiteral(":/ruby.txt"), kRubyOffX, kRubyOffY,
                  kRubyCell, kRubyCell, 70, QLatin1Char('R'));
    wait(400);

    // 6. Pixelate a slice of the icon, then undo it.
    setTool(CanvasItem::Pixelate);
    dragTo(QPointF(560, 90), QPointF(880, 260), 500);
    wait(650);
    at([this] { m_canvas->undo(); });
    wait(350);

    // 7. Select the red dot, float-move it, drop it, undo.
    setTool(CanvasItem::Selection);
    dragTo(QPointF(1044, 630), QPointF(1108, 692), 300);
    wait(220);
    dragTo(QPointF(1075, 660), QPointF(1160, 320), 550); // lift + move
    wait(300);
    click(QPointF(100, 100)); // commit
    wait(350);
    at([this] { m_canvas->undo(); });
    wait(400);

    // 8. Zoom into the mark's corner: crisp pixels + grid.
    setTool(CanvasItem::Pencil);
    for (int i = 0; i < 3; ++i) {
        at([this] { m_canvas->zoomIn(); });
        wait(320);
    }
    wait(850);
    at([this] { m_canvas->resetZoom(); });
    wait(500);

    // 9. Live Omarchy theme switch, and back.
    setTheme(QStringLiteral("flexoki-light"));
    wait(1800);
    if (!m_originalTheme.isEmpty())
        setTheme(m_originalTheme);
    wait(1500);

    // 10. Undo cascade and back — the painting rewinds and replays.
    for (int i = 0; i < 6; ++i) {
        at([this] { m_canvas->undo(); });
        wait(100);
    }
    wait(400);
    for (int i = 0; i < 6; ++i) {
        at([this] { m_canvas->redo(); });
        wait(100);
    }

    wait(1200);
    // The demo canvas is scratch: mark it clean so quitting does not raise
    // the unsaved-changes prompt.
    at([this] {
        if (m_canvas->document())
            m_canvas->document()->undoStack()->setClean();
    });
}
