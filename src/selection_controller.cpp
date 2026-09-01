#include "selection_controller.h"

#include "commands/draw_command.h"
#include "document.h"

#include <QPainter>

SelectionController::SelectionController(Document *document)
    : m_document(document)
{
}

void SelectionController::setDocument(Document *document)
{
    m_document = document;
    reset();
}

void SelectionController::reset()
{
    m_state = State::None;
    m_rect = QRect();
    m_float = QImage();
    m_liftSnapshot = QImage();
    m_sourceRect = QRect();
    m_dragging = false;
    m_fromPaste = false;
}

bool SelectionController::hasSelection() const
{
    return m_state == State::Selected || m_state == State::Floating;
}

QRect SelectionController::selectionRect() const
{
    if (m_state == State::Floating)
        return floatingRect();
    if (m_state == State::Selected || m_state == State::Selecting)
        return m_rect;
    return QRect();
}

QRect SelectionController::floatingRect() const
{
    return QRect(QPoint(qRound(m_floatPos.x()), qRound(m_floatPos.y())),
                 m_float.size());
}

void SelectionController::pointerPress(const QPointF &pos)
{
    if (!m_document)
        return;

    if (m_state == State::Floating) {
        if (floatingRect().contains(pos.toPoint())) {
            m_dragging = true;
            m_dragOffset = pos - m_floatPos;
            return;
        }
        commit();
        // fall through to start a new marquee
    } else if (m_state == State::Selected && m_rect.contains(pos.toPoint())) {
        liftSelection(pos);
        return;
    }

    m_state = State::Selecting;
    m_anchor = pos.toPoint();
    m_rect = QRect();
}

void SelectionController::pointerMove(const QPointF &pos)
{
    if (m_state == State::Selecting) {
        // QRectF(p1, p2) treats p2 as an exclusive edge (width = x2 - x1),
        // matching the raster intuition that dragging 5 → 15 selects 10 px.
        m_rect = QRectF(m_anchor, pos)
                     .normalized()
                     .toAlignedRect()
                     .intersected(m_document->image().rect());
    } else if (m_state == State::Floating && m_dragging) {
        m_floatPos = pos - m_dragOffset;
    }
}

void SelectionController::pointerRelease(const QPointF &)
{
    if (m_state == State::Selecting) {
        // A click without a drag leaves no usable region: back to None.
        m_state = m_rect.width() >= 1 && m_rect.height() >= 1 ? State::Selected
                                                              : State::None;
    } else if (m_state == State::Floating) {
        m_dragging = false;
    }
}

void SelectionController::liftSelection(const QPointF &grabPos)
{
    QImage &image = m_document->image();

    m_liftSnapshot = image; // copy-on-write
    m_sourceRect = m_rect;
    m_float = image.copy(m_rect);
    m_floatPos = m_rect.topLeft();
    m_fromPaste = false;

    clearRegion(m_rect);

    m_state = State::Floating;
    m_dragging = true;
    m_dragOffset = grabPos - m_floatPos;
    m_rect = QRect();
}

void SelectionController::clearRegion(const QRect &rect)
{
    QPainter painter(&m_document->image());
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(rect, Qt::transparent);
    painter.end();
    m_document->notifyRegionChanged(rect);
}

void SelectionController::commit()
{
    if (m_state != State::Floating)
        return;

    QImage &image = m_document->image();
    const QRect target = floatingRect();

    QPainter painter(&image);
    painter.drawImage(target.topLeft(), m_float);
    painter.end();

    QRect unionRect = m_fromPaste ? target : (m_sourceRect | target);
    unionRect = unionRect.intersected(image.rect());

    if (!unionRect.isEmpty()) {
        m_document->notifyRegionChanged(unionRect);
        const QImage before = m_liftSnapshot.copy(unionRect);
        const QImage after = image.copy(unionRect);
        if (before != after) {
            m_document->undoStack()->push(new DrawCommand(
                m_document, unionRect, before, after,
                m_fromPaste ? QStringLiteral("Paste")
                            : QStringLiteral("Move selection")));
        }
    }

    // Keep a marquee around where the pixels landed.
    const QRect landed = target.intersected(image.rect());
    m_float = QImage();
    m_liftSnapshot = QImage();
    m_sourceRect = QRect();
    m_dragging = false;
    m_fromPaste = false;
    m_rect = landed;
    m_state = landed.isEmpty() ? State::None : State::Selected;
}

void SelectionController::cancelFloating()
{
    if (m_state != State::Floating)
        return;

    if (!m_fromPaste && !m_sourceRect.isEmpty()) {
        // Put the lifted pixels back where they came from.
        QPainter painter(&m_document->image());
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawImage(m_sourceRect.topLeft(), m_liftSnapshot, m_sourceRect);
        painter.end();
        m_document->notifyRegionChanged(m_sourceRect);
        m_rect = m_sourceRect;
        m_state = State::Selected;
    } else {
        m_rect = QRect();
        m_state = State::None;
    }

    m_float = QImage();
    m_liftSnapshot = QImage();
    m_sourceRect = QRect();
    m_dragging = false;
    m_fromPaste = false;
}

void SelectionController::selectAll()
{
    if (!m_document)
        return;
    commit();
    m_rect = m_document->image().rect();
    m_state = State::Selected;
}

void SelectionController::deselect()
{
    commit();
    m_rect = QRect();
    m_state = State::None;
}

QImage SelectionController::copyImage() const
{
    if (m_state == State::Floating)
        return m_float;
    if (m_state == State::Selected && !m_rect.isEmpty())
        return m_document->image().copy(m_rect);
    return QImage();
}

QImage SelectionController::takeCut()
{
    commit();
    if (m_state != State::Selected || m_rect.isEmpty())
        return QImage();

    const QImage cut = m_document->image().copy(m_rect);
    const QRect rect = m_rect;
    clearRegion(rect);
    m_document->undoStack()->push(
        new DrawCommand(m_document, rect, cut,
                        m_document->image().copy(rect), QStringLiteral("Cut")));
    m_rect = QRect();
    m_state = State::None;
    return cut;
}

bool SelectionController::deleteSelection()
{
    commit();
    if (m_state != State::Selected || m_rect.isEmpty())
        return false;

    const QRect rect = m_rect;
    const QImage before = m_document->image().copy(rect);
    clearRegion(rect);
    m_document->undoStack()->push(
        new DrawCommand(m_document, rect, before,
                        m_document->image().copy(rect),
                        QStringLiteral("Delete")));
    m_rect = QRect();
    m_state = State::None;
    return true;
}

void SelectionController::paste(const QImage &image, const QPointF &center)
{
    if (!m_document || image.isNull())
        return;
    commit();

    m_float = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_liftSnapshot = m_document->image(); // copy-on-write
    m_sourceRect = QRect();
    m_fromPaste = true;

    // Centered on the given point, nudged so it stays at least partly visible.
    const QRect bounds = m_document->image().rect();
    QPointF pos = center - QPointF(m_float.width(), m_float.height()) / 2.0;
    pos.setX(qBound<qreal>(-m_float.width() + 1, pos.x(), bounds.width() - 1));
    pos.setY(qBound<qreal>(-m_float.height() + 1, pos.y(), bounds.height() - 1));
    m_floatPos = pos;

    m_dragging = false;
    m_rect = QRect();
    m_state = State::Floating;
}

QRect SelectionController::cropRect() const
{
    if (m_state == State::Selected && m_rect.width() >= 1 && m_rect.height() >= 1)
        return m_rect;
    return QRect();
}
