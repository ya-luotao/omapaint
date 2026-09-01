#pragma once

#include "canvas_item.h"

#include <QElapsedTimer>
#include <QPointF>
#include <QQuickWindow>
#include <QTimer>

#include <functional>

// Promo/testing only — not part of the product surface. Activated by the
// hidden `--demo` flag: drives the real, visible application by posting
// synthetic mouse/key events into the window on a pre-built timeline
// (there is no system-level pointer synthesis on Wayland/Hyprland).
// devtools/record-demo.sh records the result with gpu-screen-recorder.
class DemoDriver : public QObject
{
    Q_OBJECT

public:
    DemoDriver(QQuickWindow *window, CanvasItem *canvas, QObject *parent = nullptr);

    void start();

signals:
    void finished();

private:
    struct Step
    {
        int time;
        std::function<void()> fn;
    };

    // --- timeline building (m_cursor advances) ---
    void at(std::function<void()> fn);       // at current cursor
    void wait(int ms) { m_cursor += ms; }
    void press(const QPointF &imagePos);
    void release(const QPointF &imagePos);
    void moveTo(const QPointF &from, const QPointF &to, int duration);
    void dragTo(const QPointF &from, const QPointF &to, int duration);
    void click(const QPointF &imagePos);
    void typeText(const QString &text, int msPerChar);
    void setTool(CanvasItem::ToolType tool);
    void setColor(const QColor &color);
    void setFillMode(CanvasItem::ShapeFillMode mode);
    void setBrushSize(int size);
    void setTheme(const QString &name);
    void buildScript();
    // `cell` selects which glyph paints (default full block); art files may
    // carry a second color layer under a different glyph (e.g. 'R').
    void buildBlockArt(const QString &path, qreal offX, qreal offY, qreal cellW,
                       qreal cellH, int msPerBlock, QChar cell = QChar(0x2588));

    // --- execution helpers ---
    QPointF scenePos(const QPointF &imagePos) const;
    void postMouse(QEvent::Type type, const QPointF &imagePos,
                   Qt::MouseButton button, Qt::MouseButtons buttons);

    QQuickWindow *m_window;
    CanvasItem *m_canvas;
    QList<Step> m_steps;
    int m_cursor = 0;
    int m_next = 0;
    QElapsedTimer m_clock;
    QTimer m_timer;
    QString m_originalTheme;
};
