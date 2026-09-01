#include "document.h"
#include "selection_controller.h"

#include <QPainter>
#include <QtTest>

// Exercises the floating-selection state machine per PLAN.md: marquee,
// lift/move/commit as ONE undo command, cancel semantics for move vs paste,
// fully-off-canvas commits, cut/delete, and zero-size selections.
class TestSelection : public QObject
{
    Q_OBJECT

private slots:
    void marqueeCreatedByDrag();
    void clickWithoutDragLeavesNoSelection();
    void moveCommitIsOneUndoStep();
    void cancelMoveRestoresPixels();
    void cancelPasteDiscards();
    void commitFullyOutsideDeletesSource();
    void pasteFullyOutsideIsNoOp();
    void cutClearsAndReturnsPixels();
    void deleteClearsRegion();
    void selectAllAndCrop();
    void toolCommitKeepsMarqueeAtTarget();
};

namespace {

// 40x40 white canvas with a red 10x10 square at (5,5).
void makeTestDoc(Document &doc)
{
    doc.newDocument(40, 40);
    QPainter painter(&doc.image());
    painter.fillRect(5, 5, 10, 10, Qt::red);
    painter.end();
}

void dragSelect(SelectionController &sel, const QPoint &from, const QPoint &to)
{
    sel.pointerPress(from);
    sel.pointerMove(to);
    sel.pointerRelease(to);
}

} // namespace

void TestSelection::marqueeCreatedByDrag()
{
    Document doc;
    makeTestDoc(doc);
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));

    QCOMPARE(sel.state(), SelectionController::State::Selected);
    QCOMPARE(sel.selectionRect(), QRect(5, 5, 10, 10));
    // Marquee alone touches no pixels and pushes no command.
    QVERIFY(!doc.canUndo());
    QCOMPARE(doc.image().pixelColor(6, 6), QColor(Qt::red));
}

void TestSelection::clickWithoutDragLeavesNoSelection()
{
    Document doc;
    makeTestDoc(doc);
    SelectionController sel(&doc);

    sel.pointerPress(QPoint(20, 20));
    sel.pointerRelease(QPoint(20, 20));

    QCOMPARE(sel.state(), SelectionController::State::None);
    QVERIFY(sel.cropRect().isNull());
    QVERIFY(!sel.deleteSelection());
}

void TestSelection::moveCommitIsOneUndoStep()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));

    // Grab inside the marquee, drag by (+20,+20), drop, commit.
    sel.pointerPress(QPoint(10, 10));
    QVERIFY(sel.isFloating());
    sel.pointerMove(QPoint(30, 30));
    sel.pointerRelease(QPoint(30, 30));
    sel.commit();

    QCOMPARE(doc.image().pixelColor(26, 26), QColor(Qt::red)); // moved
    QCOMPARE(doc.image().pixelColor(6, 6).alpha(), 0);         // source cleared

    // One single undo restores the original image.
    doc.undo();
    QCOMPARE(doc.image(), original);
    QVERIFY(!doc.canUndo());

    doc.redo();
    QCOMPARE(doc.image().pixelColor(26, 26), QColor(Qt::red));
}

void TestSelection::cancelMoveRestoresPixels()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    sel.pointerPress(QPoint(10, 10));
    sel.pointerMove(QPoint(30, 30));
    sel.pointerRelease(QPoint(30, 30));

    sel.cancelFloating();

    QCOMPARE(doc.image(), original);
    QVERIFY(!doc.canUndo());
    // The marquee is back where it was.
    QCOMPARE(sel.state(), SelectionController::State::Selected);
    QCOMPARE(sel.selectionRect(), QRect(5, 5, 10, 10));
}

void TestSelection::cancelPasteDiscards()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    QImage blue(6, 6, QImage::Format_ARGB32_Premultiplied);
    blue.fill(Qt::blue);
    sel.paste(blue, QPointF(20, 20));
    QVERIFY(sel.isFloating());

    sel.cancelFloating();

    QCOMPARE(doc.image(), original);
    QCOMPARE(sel.state(), SelectionController::State::None);
    QVERIFY(!doc.canUndo());
}

void TestSelection::commitFullyOutsideDeletesSource()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    sel.pointerPress(QPoint(10, 10));
    sel.pointerMove(QPoint(200, 200)); // far off canvas
    sel.pointerRelease(QPoint(200, 200));
    sel.commit();

    QCOMPARE(doc.image().pixelColor(6, 6).alpha(), 0);
    QVERIFY(doc.canUndo());
    doc.undo();
    QCOMPARE(doc.image(), original);
}

void TestSelection::pasteFullyOutsideIsNoOp()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    QImage blue(6, 6, QImage::Format_ARGB32_Premultiplied);
    blue.fill(Qt::blue);
    sel.paste(blue, QPointF(20, 20));
    // Drag it entirely off the canvas (paste clamps its initial position,
    // so move it away by pointer).
    sel.pointerPress(QPointF(20, 20));
    sel.pointerMove(QPointF(500, 500));
    sel.pointerRelease(QPointF(500, 500));
    sel.commit();

    QCOMPARE(doc.image(), original);
    QVERIFY(!doc.canUndo());
}

void TestSelection::cutClearsAndReturnsPixels()
{
    Document doc;
    makeTestDoc(doc);
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    const QImage cut = sel.takeCut();

    QCOMPARE(cut.size(), QSize(10, 10));
    QCOMPARE(cut.pixelColor(1, 1), QColor(Qt::red));
    QCOMPARE(doc.image().pixelColor(6, 6).alpha(), 0);
    QCOMPARE(sel.state(), SelectionController::State::None);

    doc.undo();
    QCOMPARE(doc.image().pixelColor(6, 6), QColor(Qt::red));
}

void TestSelection::deleteClearsRegion()
{
    Document doc;
    makeTestDoc(doc);
    const QImage original = doc.image();
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    QVERIFY(sel.deleteSelection());

    QCOMPARE(doc.image().pixelColor(6, 6).alpha(), 0);
    doc.undo();
    QCOMPARE(doc.image(), original);
}

void TestSelection::selectAllAndCrop()
{
    Document doc;
    makeTestDoc(doc);
    SelectionController sel(&doc);

    sel.selectAll();
    QCOMPARE(sel.selectionRect(), QRect(0, 0, 40, 40));
    // Cropping to the whole image is a no-op.
    doc.cropTo(sel.cropRect());
    QVERIFY(!doc.canUndo());

    // Pressing inside a selection lifts it, so deselect before re-selecting.
    sel.deselect();
    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    doc.cropTo(sel.cropRect());
    QCOMPARE(doc.imageSize(), QSize(10, 10));
    QCOMPARE(doc.image().pixelColor(1, 1), QColor(Qt::red));

    doc.undo();
    QCOMPARE(doc.imageSize(), QSize(40, 40));
    doc.redo();
    QCOMPARE(doc.imageSize(), QSize(10, 10));
}

void TestSelection::toolCommitKeepsMarqueeAtTarget()
{
    Document doc;
    makeTestDoc(doc);
    SelectionController sel(&doc);

    dragSelect(sel, QPoint(5, 5), QPoint(15, 15));
    sel.pointerPress(QPoint(10, 10));
    sel.pointerMove(QPoint(30, 30));
    sel.pointerRelease(QPoint(30, 30));
    sel.commit();

    QCOMPARE(sel.state(), SelectionController::State::Selected);
    QCOMPARE(sel.selectionRect(), QRect(25, 25, 10, 10));
}

QTEST_MAIN(TestSelection)
#include "tst_selection.moc"
