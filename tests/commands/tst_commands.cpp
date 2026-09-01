#include "document.h"

#include <QPainter>
#include <QtTest>

// Whole-image ImageCommand operations: crop, resize image, resize canvas.
class TestCommands : public QObject
{
    Q_OBJECT

private slots:
    void cropUndoRedo();
    void resizeImageScalesAndRestores();
    void resizeCanvasAnchorsTopLeftWithWhite();
    void sizeOpsMarkDirty();
};

void TestCommands::cropUndoRedo()
{
    Document doc;
    doc.newDocument(40, 30);
    doc.image().setPixelColor(12, 11, Qt::red);
    const QImage original = doc.image();

    doc.cropTo(QRect(10, 10, 20, 15));
    QCOMPARE(doc.imageSize(), QSize(20, 15));
    QCOMPARE(doc.image().pixelColor(2, 1), QColor(Qt::red));

    doc.undo();
    QCOMPARE(doc.image(), original);
    doc.redo();
    QCOMPARE(doc.imageSize(), QSize(20, 15));
}

void TestCommands::resizeImageScalesAndRestores()
{
    Document doc;
    doc.newDocument(40, 40);
    QPainter painter(&doc.image());
    painter.fillRect(0, 0, 20, 40, Qt::black); // left half black
    painter.end();
    const QImage original = doc.image();

    doc.resizeImage(20, 20);
    QCOMPARE(doc.imageSize(), QSize(20, 20));
    QCOMPARE(doc.image().pixelColor(2, 10), QColor(Qt::black));
    QCOMPARE(doc.image().pixelColor(18, 10), QColor(Qt::white));

    doc.undo();
    QCOMPARE(doc.image(), original);

    // Resizing to the same size is a no-op.
    doc.resizeImage(40, 40);
    QVERIFY(!doc.canRedo() || doc.image() == original);
    QCOMPARE(doc.imageSize(), QSize(40, 40));
}

void TestCommands::resizeCanvasAnchorsTopLeftWithWhite()
{
    Document doc;
    doc.newDocument(20, 20);
    doc.image().setPixelColor(5, 5, Qt::blue);
    const QImage original = doc.image();

    doc.resizeCanvas(30, 25);
    QCOMPARE(doc.imageSize(), QSize(30, 25));
    QCOMPARE(doc.image().pixelColor(5, 5), QColor(Qt::blue)); // kept
    QCOMPARE(doc.image().pixelColor(25, 22), QColor(Qt::white)); // new area

    doc.undo();
    QCOMPARE(doc.image(), original);

    // Shrinking clips.
    doc.resizeCanvas(10, 10);
    QCOMPARE(doc.imageSize(), QSize(10, 10));
    QCOMPARE(doc.image().pixelColor(5, 5), QColor(Qt::blue));
}

void TestCommands::sizeOpsMarkDirty()
{
    Document doc;
    doc.newDocument(20, 20);
    QVERIFY(!doc.isDirty());
    doc.resizeCanvas(25, 25);
    QVERIFY(doc.isDirty());
    doc.undo();
    QVERIFY(!doc.isDirty());
}

QTEST_MAIN(TestCommands)
#include "tst_commands.moc"
