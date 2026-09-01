#pragma once

#include "document.h"
#include "tools/eraser_tool.h"
#include "tools/pencil_tool.h"

#include <QColor>
#include <QQuickPaintedItem>

// Renders the document image 1:1 and turns pointer input into tool strokes.
// One stroke = one DrawCommand: on press the whole image is snapshotted
// (cheap — QImage is copy-on-write, the deep copy only happens lazily when
// the tool first paints), on release before/after are cropped to the
// accumulated damage rect and pushed onto the undo stack.
class CanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Document *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(ToolType tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor
                   NOTIFY foregroundColorChanged)

public:
    enum ToolType {
        Pencil,
        Eraser,
    };
    Q_ENUM(ToolType)

    explicit CanvasItem(QQuickItem *parent = nullptr);

    Document *document() const { return m_document; }
    void setDocument(Document *document);

    ToolType tool() const { return m_tool; }
    void setTool(ToolType tool);

    QColor foregroundColor() const { return m_foregroundColor; }
    void setForegroundColor(const QColor &color);

    void paint(QPainter *painter) override;

signals:
    void documentChanged();
    void toolChanged();
    void foregroundColorChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Tool *activeTool();
    void syncSize();
    void addDamage(const QRect &rect);
    void finishStroke();

    Document *m_document = nullptr;
    QList<QMetaObject::Connection> m_documentConnections;

    ToolType m_tool = Pencil;
    QColor m_foregroundColor = Qt::black;

    PencilTool m_pencil;
    EraserTool m_eraser;

    bool m_stroking = false;
    QImage m_beforeStroke;
    QRect m_damage;
};
