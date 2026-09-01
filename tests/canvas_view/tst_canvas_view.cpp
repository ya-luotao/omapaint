#include "canvas_view.h"

#include <QtTest>

class TestCanvasView : public QObject
{
    Q_OBJECT

private slots:
    void imageSmallerThanViewportIsCentered();
    void imageLargerThanViewportFollowsPan();
    void mixedAxes();
    void roundTripItemImageItem();
    void zoomScalesMapping();
    void snapAlignsToDevicePixels();
};

void TestCanvasView::imageSmallerThanViewportIsCentered()
{
    const QPointF o = CanvasView::origin(QSizeF(1000, 800), QSize(400, 200), 1.0,
                                         QPointF(0, 0));
    QCOMPARE(o, QPointF(300, 300));
}

void TestCanvasView::imageLargerThanViewportFollowsPan()
{
    const QPointF o = CanvasView::origin(QSizeF(500, 400), QSize(400, 200), 4.0,
                                         QPointF(120, 80));
    QCOMPARE(o, QPointF(-120, -80));
}

void TestCanvasView::mixedAxes()
{
    // Wider than the viewport, shorter than it: panned in x, centered in y.
    const QPointF o = CanvasView::origin(QSizeF(500, 400), QSize(600, 100), 1.0,
                                         QPointF(50, 0));
    QCOMPARE(o.x(), -50.0);
    QCOMPARE(o.y(), 150.0);
}

void TestCanvasView::roundTripItemImageItem()
{
    const QPointF origin(37.5, -12.25);
    const qreal zoom = 4.0;

    const QPointF itemPos(123.0, 456.0);
    const QPointF imagePos = CanvasView::toImage(itemPos, origin, zoom);
    const QRectF back = CanvasView::fromImage(QRectF(imagePos, QSizeF(0, 0)),
                                              origin, zoom);
    QCOMPARE(back.topLeft(), itemPos);
}

void TestCanvasView::zoomScalesMapping()
{
    const QPointF origin(0, 0);
    QCOMPARE(CanvasView::toImage(QPointF(80, 40), origin, 4.0), QPointF(20, 10));
    QCOMPARE(CanvasView::fromImage(QRectF(10, 20, 30, 40), origin, 2.0),
             QRectF(20, 40, 60, 80));
}

void TestCanvasView::snapAlignsToDevicePixels()
{
    // At dpr 1.25, device pixels sit every 0.8 logical units.
    const qreal snapped = CanvasView::snapToDevicePixels(1.0, 1.25);
    QCOMPARE(snapped, 0.8);

    // Snapped values land exactly on the device pixel grid.
    QCOMPARE(qRound(snapped * 1.25 * 1000) % 1000, 0);

    // dpr 1 and 2 keep already-aligned values unchanged.
    QCOMPARE(CanvasView::snapToDevicePixels(3.0, 1.0), 3.0);
    QCOMPARE(CanvasView::snapToDevicePixels(3.5, 2.0), 3.5);
}

QTEST_MAIN(TestCanvasView)
#include "tst_canvas_view.moc"
