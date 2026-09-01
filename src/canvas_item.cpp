#include "canvas_item.h"

#include "canvas_view.h"
#include "commands/draw_command.h"

#include <QMouseEvent>
#include <QPainter>
#include <QQuickWindow>
#include <QtMath>

namespace {

constexpr qreal kZoomSteps[] = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
constexpr qreal kGridMinZoom = 8.0;
constexpr int kMaxBrushSize = 64;

qreal nextZoomStep(qreal zoom, int direction)
{
    if (direction > 0) {
        for (qreal step : kZoomSteps) {
            if (step > zoom + 0.001)
                return step;
        }
        return kZoomSteps[std::size(kZoomSteps) - 1];
    }
    for (auto it = std::rbegin(kZoomSteps); it != std::rend(kZoomSteps); ++it) {
        if (*it < zoom - 0.001)
            return *it;
    }
    return kZoomSteps[0];
}

} // namespace

CanvasItem::CanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    connect(&m_clipboard, &Clipboard::changed, this, &CanvasItem::canPasteChanged);
}

void CanvasItem::setDocument(Document *document)
{
    if (m_document == document)
        return;

    for (const auto &connection : std::as_const(m_documentConnections))
        disconnect(connection);
    m_documentConnections.clear();

    m_document = document;
    m_selection.setDocument(document);
    emit selectionChanged();

    if (m_document) {
        m_documentConnections << connect(m_document, &Document::imageChanged, this, [this] {
            m_stroking = false;
            m_selection.reset();
            emit selectionChanged();
            update();
        });
        m_documentConnections << connect(
            m_document, &Document::regionChanged, this,
            [this](const QRect &rect) { updateImageRegion(rect); });
    }

    update();
    emit documentChanged();
}

void CanvasItem::setTool(ToolType tool)
{
    if (m_tool == tool)
        return;
    // Leaving the selection tool commits any float and drops the marquee.
    if (m_tool == Selection) {
        m_selection.deselect();
        selectionUpdated();
    }
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

void CanvasItem::setBrushSize(int size)
{
    size = qBound(1, size, kMaxBrushSize);
    if (m_brushSize == size)
        return;
    m_brushSize = size;
    emit brushSizeChanged();
}

void CanvasItem::setZoom(qreal zoom)
{
    zoom = qBound(kZoomSteps[0], zoom, kZoomSteps[std::size(kZoomSteps) - 1]);
    if (qFuzzyCompare(m_zoom, zoom))
        return;
    m_zoom = zoom;
    emit zoomChanged();
    update();
}

void CanvasItem::setPanX(qreal x)
{
    if (qFuzzyCompare(m_pan.x() + 1.0, x + 1.0))
        return;
    m_pan.setX(x);
    emit panChanged();
    update();
}

void CanvasItem::setPanY(qreal y)
{
    if (qFuzzyCompare(m_pan.y() + 1.0, y + 1.0))
        return;
    m_pan.setY(y);
    emit panChanged();
    update();
}

void CanvasItem::setPixelGrid(bool enabled)
{
    if (m_pixelGrid == enabled)
        return;
    m_pixelGrid = enabled;
    emit pixelGridChanged();
    update();
}

void CanvasItem::zoomIn()
{
    zoomTo(nextZoomStep(m_zoom, 1), QPointF(width() / 2, height() / 2));
}

void CanvasItem::zoomOut()
{
    zoomTo(nextZoomStep(m_zoom, -1), QPointF(width() / 2, height() / 2));
}

void CanvasItem::resetZoom()
{
    zoomTo(1.0, QPointF(width() / 2, height() / 2));
}

void CanvasItem::paint(QPainter *painter)
{
    if (!m_document)
        return;

    const QPointF o = snappedOrigin();

    painter->save();
    painter->translate(o);
    painter->scale(m_zoom, m_zoom);
    // Nearest neighbor when zoomed in (crisp pixels), smooth when zoomed out.
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_zoom < 1.0);
    painter->drawImage(0, 0, m_document->image());
    if (m_selection.isFloating())
        painter->drawImage(m_selection.floatingPos(), m_selection.floatingImage());
    painter->restore();

    if (m_pixelGrid && m_zoom >= kGridMinZoom)
        drawPixelGrid(painter);

    drawSelectionOverlay(painter);
}

void CanvasItem::drawSelectionOverlay(QPainter *painter) const
{
    const QRect rect = m_selection.selectionRect();
    if (rect.isEmpty() && m_selection.state() != SelectionController::State::Selecting)
        return;
    if (rect.isEmpty())
        return;

    const QRectF r = CanvasView::fromImage(QRectF(rect), snappedOrigin(), m_zoom);

    // Static two-tone marching ants: solid white underneath, dashed black on
    // top, visible on any background without an animation timer.
    painter->setBrush(Qt::NoBrush);
    QPen white(Qt::white);
    white.setWidth(0); // cosmetic
    painter->setPen(white);
    painter->drawRect(r);

    QPen black(Qt::black);
    black.setWidth(0);
    black.setStyle(Qt::DashLine);
    painter->setPen(black);
    painter->drawRect(r);
}

void CanvasItem::drawPixelGrid(QPainter *painter) const
{
    const QPointF o = snappedOrigin();
    const QSize imageSize = m_document->imageSize();

    // Visible image-pixel range.
    const QPointF topLeft = CanvasView::toImage(QPointF(0, 0), o, m_zoom);
    const QPointF bottomRight =
        CanvasView::toImage(QPointF(width(), height()), o, m_zoom);
    const int x0 = qMax(0, qFloor(topLeft.x()));
    const int x1 = qMin(imageSize.width(), qCeil(bottomRight.x()));
    const int y0 = qMax(0, qFloor(topLeft.y()));
    const int y1 = qMin(imageSize.height(), qCeil(bottomRight.y()));
    if (x0 >= x1 || y0 >= y1)
        return;

    QPen gridPen(QColor(128, 128, 128, 96));
    gridPen.setWidth(0); // cosmetic: one device pixel regardless of scaling
    painter->setPen(gridPen);

    const qreal top = o.y() + y0 * m_zoom;
    const qreal bottom = o.y() + y1 * m_zoom;
    for (int x = x0; x <= x1; ++x) {
        const qreal ix = o.x() + x * m_zoom;
        painter->drawLine(QPointF(ix, top), QPointF(ix, bottom));
    }
    const qreal left = o.x() + x0 * m_zoom;
    const qreal right = o.x() + x1 * m_zoom;
    for (int y = y0; y <= y1; ++y) {
        const qreal iy = o.y() + y * m_zoom;
        painter->drawLine(QPointF(left, iy), QPointF(right, iy));
    }
}

void CanvasItem::mousePressEvent(QMouseEvent *event)
{
    if (!m_document || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    // Keep the grab so the enclosing Flickable cannot steal the drag.
    setKeepMouseGrab(true);

    const QPointF imagePos = toImage(event->position());

    if (m_tool == Eyedropper) {
        m_picking = true;
        pickColor(imagePos);
        event->accept();
        return;
    }

    if (m_tool == Selection) {
        m_selection.pointerPress(imagePos);
        selectionUpdated();
        event->accept();
        return;
    }

    m_stroking = true;
    m_beforeStroke = m_document->image(); // copy-on-write snapshot
    m_damage = QRect();

    const QRect damage =
        activeTool()->begin(m_document->image(), imagePos, toolContext());
    if (!damage.isEmpty()) {
        m_damage |= damage;
        updateImageRegion(damage);
    }
    event->accept();
}

void CanvasItem::mouseMoveEvent(QMouseEvent *event)
{
    setHover(event->position(), true);

    if (m_picking) {
        pickColor(toImage(event->position()));
        event->accept();
        return;
    }
    if (m_tool == Selection) {
        m_selection.pointerMove(toImage(event->position()));
        selectionUpdated();
        event->accept();
        return;
    }
    if (!m_stroking) {
        event->ignore();
        return;
    }

    const QRect damage = activeTool()->move(m_document->image(),
                                            toImage(event->position()),
                                            toolContext());
    if (!damage.isEmpty()) {
        m_damage |= damage;
        updateImageRegion(damage);
    }
    event->accept();
}

void CanvasItem::mouseReleaseEvent(QMouseEvent *event)
{
    setKeepMouseGrab(false);

    if (m_picking) {
        m_picking = false;
        event->accept();
        return;
    }
    if (m_tool == Selection) {
        m_selection.pointerRelease(toImage(event->position()));
        selectionUpdated();
        event->accept();
        return;
    }
    if (!m_stroking) {
        event->ignore();
        return;
    }

    const QRect damage = activeTool()->end(m_document->image(),
                                           toImage(event->position()),
                                           toolContext());
    if (!damage.isEmpty()) {
        m_damage |= damage;
        updateImageRegion(damage);
    }
    finishStroke();
    event->accept();
}

void CanvasItem::hoverMoveEvent(QHoverEvent *event)
{
    setHover(event->position(), true);
}

void CanvasItem::hoverLeaveEvent(QHoverEvent *)
{
    setHover(QPointF(), false);
}

void CanvasItem::wheelEvent(QWheelEvent *event)
{
    if (!(event->modifiers() & Qt::ControlModifier)) {
        event->ignore(); // let the Flickable underneath scroll
        return;
    }
    const int direction = event->angleDelta().y() > 0 ? 1 : -1;
    zoomTo(nextZoomStep(m_zoom, direction), event->position());
    event->accept();
}

Tool *CanvasItem::activeTool()
{
    switch (m_tool) {
    case Brush:
        return &m_brush;
    case Eraser:
        return &m_eraser;
    case Line:
        return &m_line;
    case Rectangle:
        return &m_rectangle;
    case Ellipse:
        return &m_ellipse;
    case Fill:
        return &m_fill;
    case Eyedropper: // handled before tools are consulted
    case Selection:  // handled by the selection controller
    case Pencil:
        break;
    }
    return &m_pencil;
}

void CanvasItem::selectionUpdated()
{
    emit selectionChanged();
    update();
}

void CanvasItem::undo()
{
    // While a float is pending, undo cancels it (nothing was committed yet);
    // only a clean canvas unwinds real history.
    if (m_selection.isFloating())
        m_selection.cancelFloating();
    else if (m_document)
        m_document->undo();
    selectionUpdated();
}

void CanvasItem::redo()
{
    m_selection.commit();
    if (m_document)
        m_document->redo();
    selectionUpdated();
}

void CanvasItem::cut()
{
    const QImage image = m_selection.takeCut();
    if (!image.isNull())
        m_clipboard.setImage(image);
    selectionUpdated();
}

void CanvasItem::copy()
{
    QImage image = m_selection.copyImage();
    if (image.isNull() && m_document)
        image = m_document->image(); // no selection: copy the whole image
    if (!image.isNull())
        m_clipboard.setImage(image);
}

void CanvasItem::paste()
{
    if (!m_document || !m_clipboard.hasImage())
        return;
    setTool(Selection);
    m_selection.paste(m_clipboard.image(),
                      toImage(QPointF(width() / 2, height() / 2)));
    selectionUpdated();
}

void CanvasItem::deleteSelection()
{
    m_selection.deleteSelection();
    selectionUpdated();
}

void CanvasItem::selectAll()
{
    if (!m_document)
        return;
    setTool(Selection);
    m_selection.selectAll();
    selectionUpdated();
}

void CanvasItem::escape()
{
    if (m_selection.isFloating())
        m_selection.cancelFloating();
    else
        m_selection.deselect();
    selectionUpdated();
}

void CanvasItem::crop()
{
    if (!m_document)
        return;
    m_selection.commit();
    const QRect rect = m_selection.cropRect();
    if (!rect.isNull())
        m_document->cropTo(rect);
    selectionUpdated();
}

void CanvasItem::commitSelection()
{
    m_selection.commit();
    selectionUpdated();
}

ToolContext CanvasItem::toolContext() const
{
    return ToolContext{m_foregroundColor, static_cast<qreal>(m_brushSize)};
}

QPointF CanvasItem::snappedOrigin() const
{
    if (!m_document)
        return QPointF();
    const QPointF o = CanvasView::origin(QSizeF(width(), height()),
                                         m_document->imageSize(), m_zoom, m_pan);
    const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
    return QPointF(CanvasView::snapToDevicePixels(o.x(), dpr),
                   CanvasView::snapToDevicePixels(o.y(), dpr));
}

QPointF CanvasItem::toImage(const QPointF &itemPos) const
{
    return CanvasView::toImage(itemPos, snappedOrigin(), m_zoom);
}

void CanvasItem::updateImageRegion(const QRect &imageRect)
{
    const QRectF itemRect =
        CanvasView::fromImage(QRectF(imageRect), snappedOrigin(), m_zoom);
    const QRect target = itemRect.toAlignedRect()
                             .adjusted(-1, -1, 1, 1)
                             .intersected(QRect(0, 0, qCeil(width()), qCeil(height())));
    if (!target.isEmpty())
        update(target);
}

void CanvasItem::zoomTo(qreal targetZoom, const QPointF &anchorItemPos)
{
    if (!m_document || qFuzzyCompare(targetZoom, m_zoom))
        return;

    const QPointF imagePos = toImage(anchorItemPos);
    setZoom(targetZoom);

    // Scroll so the anchored image point stays under the cursor.
    const QSize imageSize = m_document->imageSize();
    const qreal maxPanX = qMax(0.0, imageSize.width() * m_zoom - width());
    const qreal maxPanY = qMax(0.0, imageSize.height() * m_zoom - height());
    const qreal panX =
        qBound(0.0, imagePos.x() * m_zoom - anchorItemPos.x(), maxPanX);
    const qreal panY =
        qBound(0.0, imagePos.y() * m_zoom - anchorItemPos.y(), maxPanY);
    emit panRequest(panX, panY);
}

void CanvasItem::pickColor(const QPointF &imagePos)
{
    const QPoint p(qBound(0, qFloor(imagePos.x()), m_document->imageSize().width() - 1),
                   qBound(0, qFloor(imagePos.y()), m_document->imageSize().height() - 1));
    setForegroundColor(m_document->image().pixelColor(p));
}

void CanvasItem::setHover(const QPointF &itemPos, bool valid)
{
    QPointF imagePos;
    if (valid && m_document) {
        imagePos = toImage(itemPos);
        valid = QRectF(QPointF(0, 0), QSizeF(m_document->imageSize()))
                    .contains(imagePos);
    }
    const QPoint pixel(qFloor(imagePos.x()), qFloor(imagePos.y()));
    if (m_hoverValid == valid && (!valid || m_hoverImagePos == QPointF(pixel)))
        return;
    m_hoverValid = valid;
    m_hoverImagePos = QPointF(pixel);
    emit hoverChanged();
}

void CanvasItem::finishStroke()
{
    m_stroking = false;

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
