#pragma once

#include "clipboard.h"
#include "document.h"
#include "selection_controller.h"
#include "tools/brush_tool.h"
#include "tools/ellipse_tool.h"
#include "tools/eraser_tool.h"
#include "tools/fill_tool.h"
#include "tools/line_tool.h"
#include "tools/pencil_tool.h"
#include "tools/rectangle_tool.h"

#include <QColor>
#include <QQuickPaintedItem>

// Viewport-sized view of the document image: sits on top of a Flickable that
// only supplies scrollbars and pan state. The item never grows with zoom
// (a zoomed-out texture of image-size * zoom would be enormous); instead
// paint() translates by the pan/centering origin and scales by the zoom.
//
// One stroke = one DrawCommand: on press the whole image is snapshotted
// (cheap — QImage is copy-on-write), on release before/after are cropped to
// the accumulated damage rect and pushed onto the undo stack.
class CanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Document *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(ToolType tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor
                   NOTIFY foregroundColorChanged)
    Q_PROPERTY(int brushSize READ brushSize WRITE setBrushSize NOTIFY brushSizeChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panChanged)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panChanged)
    Q_PROPERTY(bool pixelGrid READ pixelGrid WRITE setPixelGrid NOTIFY pixelGridChanged)
    Q_PROPERTY(QPointF hoverImagePos READ hoverImagePos NOTIFY hoverChanged)
    Q_PROPERTY(bool hoverValid READ hoverValid NOTIFY hoverChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(bool floating READ isFloating NOTIFY selectionChanged)
    Q_PROPERTY(QRect selectionRect READ selectionRect NOTIFY selectionChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY canPasteChanged)

public:
    enum ToolType {
        Pencil,
        Brush,
        Eraser,
        Line,
        Rectangle,
        Ellipse,
        Fill,
        Eyedropper,
        Selection,
    };
    Q_ENUM(ToolType)

    explicit CanvasItem(QQuickItem *parent = nullptr);

    Document *document() const { return m_document; }
    void setDocument(Document *document);

    ToolType tool() const { return m_tool; }
    void setTool(ToolType tool);

    QColor foregroundColor() const { return m_foregroundColor; }
    void setForegroundColor(const QColor &color);

    int brushSize() const { return m_brushSize; }
    void setBrushSize(int size);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);

    qreal panX() const { return m_pan.x(); }
    void setPanX(qreal x);
    qreal panY() const { return m_pan.y(); }
    void setPanY(qreal y);

    bool pixelGrid() const { return m_pixelGrid; }
    void setPixelGrid(bool enabled);

    QPointF hoverImagePos() const { return m_hoverImagePos; }
    bool hoverValid() const { return m_hoverValid; }

    bool hasSelection() const { return m_selection.hasSelection(); }
    bool isFloating() const { return m_selection.isFloating(); }
    QRect selectionRect() const { return m_selection.selectionRect(); }
    bool canPaste() const { return m_clipboard.hasImage(); }

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetZoom();

    // Selection and clipboard operations. undo/redo MUST be routed through
    // these (not Document) so undo-while-floating cancels the float instead
    // of unwinding history underneath it.
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void escape(); // cancel a float, else drop the marquee
    Q_INVOKABLE void crop();
    Q_INVOKABLE void commitSelection();

    void paint(QPainter *painter) override;

signals:
    void documentChanged();
    void toolChanged();
    void foregroundColorChanged();
    void brushSizeChanged();
    void zoomChanged();
    void panChanged();
    void pixelGridChanged();
    void hoverChanged();
    void selectionChanged();
    void canPasteChanged();
    // Asks the enclosing Flickable to scroll to the given content offset
    // (emitted when zooming so the anchor point stays put).
    void panRequest(qreal x, qreal y);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Tool *activeTool();
    ToolContext toolContext() const;
    QPointF snappedOrigin() const;
    QPointF toImage(const QPointF &itemPos) const;
    void updateImageRegion(const QRect &imageRect);
    void zoomTo(qreal targetZoom, const QPointF &anchorItemPos);
    void pickColor(const QPointF &imagePos);
    void setHover(const QPointF &itemPos, bool valid);
    void finishStroke();
    void drawPixelGrid(QPainter *painter) const;
    void drawSelectionOverlay(QPainter *painter) const;
    void selectionUpdated();

    Document *m_document = nullptr;
    QList<QMetaObject::Connection> m_documentConnections;

    ToolType m_tool = Pencil;
    QColor m_foregroundColor = Qt::black;
    int m_brushSize = 6;
    qreal m_zoom = 1.0;
    QPointF m_pan;
    bool m_pixelGrid = true;

    QPointF m_hoverImagePos;
    bool m_hoverValid = false;

    PencilTool m_pencil;
    BrushTool m_brush;
    EraserTool m_eraser;
    LineTool m_line;
    RectangleTool m_rectangle;
    EllipseTool m_ellipse;
    FillTool m_fill;

    SelectionController m_selection;
    Clipboard m_clipboard;

    bool m_stroking = false;
    bool m_picking = false;
    QImage m_beforeStroke;
    QRect m_damage;
};
