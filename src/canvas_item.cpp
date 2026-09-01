#include "canvas_item.h"

#include "commands/draw_command.h"

#include <QMouseEvent>
#include <QPainter>

CanvasItem::CanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}

void CanvasItem::setDocument(Document *document)
{
    if (m_document == document)
        return;

    for (const auto &connection : std::as_const(m_documentConnections))
        disconnect(connection);
    m_documentConnections.clear();

    m_document = document;

    if (m_document) {
        m_documentConnections << connect(m_document, &Document::imageChanged, this, [this] {
            m_stroking = false;
            syncSize();
            update();
        });
        m_documentConnections << connect(m_document, &Document::regionChanged, this,
                                         [this](const QRect &rect) { update(rect); });
        syncSize();
    }

    update();
    emit documentChanged();
}

void CanvasItem::setTool(ToolType tool)
{
    if (m_tool == tool)
        return;
    m_tool = tool;
    emit toolChanged();
}

void CanvasItem::setForegroundColor(const QColor &color)
{
    if (m_foregroundColor == color)
        return;
    m_foregroundColor = color;
    emit foregroundColorChanged();
}

void CanvasItem::paint(QPainter *painter)
{
    if (!m_document)
        return;
    painter->drawImage(0, 0, m_document->image());
}

void CanvasItem::mousePressEvent(QMouseEvent *event)
{
    if (!m_document || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    // Keep the grab so an enclosing Flickable cannot steal the drag.
    setKeepMouseGrab(true);

    m_stroking = true;
    m_beforeStroke = m_document->image(); // copy-on-write snapshot
    m_damage = QRect();

    addDamage(activeTool()->begin(m_document->image(), event->position(), m_foregroundColor));
    event->accept();
}

void CanvasItem::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_stroking) {
        event->ignore();
        return;
    }
    addDamage(activeTool()->move(m_document->image(), event->position(), m_foregroundColor));
    event->accept();
}

void CanvasItem::mouseReleaseEvent(QMouseEvent *event)
{
    setKeepMouseGrab(false);
    if (!m_stroking) {
        event->ignore();
        return;
    }
    finishStroke();
    event->accept();
}

Tool *CanvasItem::activeTool()
{
    switch (m_tool) {
    case Eraser:
        return &m_eraser;
    case Pencil:
        break;
    }
    return &m_pencil;
}

void CanvasItem::syncSize()
{
    const QSize size = m_document ? m_document->imageSize() : QSize();
    setImplicitWidth(size.width());
    setImplicitHeight(size.height());
}

void CanvasItem::addDamage(const QRect &rect)
{
    if (rect.isEmpty())
        return;
    m_damage |= rect;
    update(rect);
}

void CanvasItem::finishStroke()
{
    m_stroking = false;
    activeTool()->end();

    if (m_damage.isEmpty() || !m_document) {
        m_beforeStroke = QImage();
        return;
    }

    const QImage before = m_beforeStroke.copy(m_damage);
    const QImage after = m_document->image().copy(m_damage);
    m_beforeStroke = QImage();

    if (before == after)
        return;

    m_document->undoStack()->push(
        new DrawCommand(m_document, m_damage, before, after, activeTool()->name()));
}
