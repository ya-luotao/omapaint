#include "commands/draw_command.h"
#include "document.h"
#include "tools/eraser_tool.h"
#include "tools/pencil_tool.h"

#include <QtTest>

class TestUndo : public QObject
{
    Q_OBJECT

private slots:
    void undoRestoresOriginal();
    void redoRestoresResult();
    void multipleStrokesUndoInOrder();
    void eraserStrokeUndoRestoresPixels();
};

// Mirrors CanvasItem's stroke commit: snapshot, draw, push region command.
static void commitStroke(Document &doc, Tool &tool, const QPointF &from,
                         const QPointF &to, const QColor &color)
{
    const ToolContext ctx{color, 8};
    const QImage before = doc.image();
    QRect damage = tool.begin(doc.image(), from, ctx);
    damage |= tool.move(doc.image(), to, ctx);
    tool.end(doc.image(), to, ctx);
    doc.undoStack()->push(new DrawCommand(&doc, damage, before.copy(damage),
                                          doc.image().copy(damage), tool.name()));
}

void TestUndo::undoRestoresOriginal()
{
    Document doc;
    doc.newDocument(50, 50);
    const QImage original = doc.image();

    PencilTool pencil;
    commitStroke(doc, pencil, QPointF(5, 5), QPointF(40, 30), Qt::blue);
    QVERIFY(doc.image() != original);

    doc.undo();
    QCOMPARE(doc.image(), original);
}

void TestUndo::redoRestoresResult()
{
    Document doc;
    doc.newDocument(50, 50);

    PencilTool pencil;
    commitStroke(doc, pencil, QPointF(10, 10), QPointF(35, 12), Qt::red);
    const QImage afterStroke = doc.image();

    doc.undo();
    doc.redo();
    QCOMPARE(doc.image(), afterStroke);
}

void TestUndo::multipleStrokesUndoInOrder()
{
    Document doc;
    doc.newDocument(50, 50);
    const QImage original = doc.image();

    PencilTool pencil;
    commitStroke(doc, pencil, QPointF(5, 5), QPointF(20, 5), Qt::red);
    const QImage afterFirst = doc.image();
    commitStroke(doc, pencil, QPointF(5, 15), QPointF(20, 15), Qt::green);

    doc.undo();
    QCOMPARE(doc.image(), afterFirst);
    doc.undo();
    QCOMPARE(doc.image(), original);
    doc.redo();
    QCOMPARE(doc.image(), afterFirst);
}

void TestUndo::eraserStrokeUndoRestoresPixels()
{
    Document doc;
    doc.newDocument(50, 50);
    const QImage original = doc.image();

    EraserTool eraser;
    commitStroke(doc, eraser, QPointF(25, 25), QPointF(30, 30), Qt::black);
    QCOMPARE(doc.image().pixelColor(25, 25).alpha(), 0);

    doc.undo();
    QCOMPARE(doc.image(), original);
    QCOMPARE(doc.image().pixelColor(25, 25), QColor(Qt::white));
}

QTEST_MAIN(TestUndo)
#include "tst_undo.moc"
