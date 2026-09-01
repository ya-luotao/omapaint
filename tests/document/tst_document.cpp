#include "commands/draw_command.h"
#include "document.h"
#include "tools/pencil_tool.h"

#include <QTemporaryDir>
#include <QtTest>

class TestDocument : public QObject
{
    Q_OBJECT

private slots:
    void newDocumentIsWhiteAndClean();
    void strokeMakesDirtySaveMakesClean();
    void loadResetsHistoryAndPath();
    void fileNameFollowsPath();
};

// Draws one pencil stroke and commits it as an undo command, mirroring what
// CanvasItem does on pointer release.
static void commitStroke(Document &doc, const QPointF &from, const QPointF &to,
                         const QColor &color)
{
    PencilTool pencil;
    const QImage before = doc.image();
    QRect damage = pencil.begin(doc.image(), from, color);
    damage |= pencil.move(doc.image(), to, color);
    pencil.end();
    doc.undoStack()->push(new DrawCommand(&doc, damage, before.copy(damage),
                                          doc.image().copy(damage), "Pencil"));
}

void TestDocument::newDocumentIsWhiteAndClean()
{
    Document doc;
    doc.newDocument(64, 48);

    QCOMPARE(doc.imageSize(), QSize(64, 48));
    QCOMPARE(doc.image().format(), QImage::Format_ARGB32_Premultiplied);
    QCOMPARE(doc.image().pixelColor(0, 0), QColor(Qt::white));
    QCOMPARE(doc.image().pixelColor(63, 47), QColor(Qt::white));
    QVERIFY(!doc.isDirty());
    QVERIFY(!doc.canUndo());
    QVERIFY(doc.filePath().isEmpty());
}

void TestDocument::strokeMakesDirtySaveMakesClean()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Document doc;
    doc.newDocument(32, 32);
    QVERIFY(!doc.isDirty());

    commitStroke(doc, QPointF(4, 4), QPointF(20, 4), Qt::black);
    QVERIFY(doc.isDirty());
    QVERIFY(doc.canUndo());

    const QString path = dir.filePath("out.png");
    QVERIFY(doc.saveFile(path));
    QVERIFY(!doc.isDirty());
    QCOMPARE(doc.filePath(), path);

    // Undoing past the save point makes the document dirty again;
    // redoing back to it makes it clean.
    doc.undo();
    QVERIFY(doc.isDirty());
    doc.redo();
    QVERIFY(!doc.isDirty());
}

void TestDocument::loadResetsHistoryAndPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("in.png");

    QImage source(16, 16, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::red);
    QVERIFY(source.save(path));

    Document doc;
    commitStroke(doc, QPointF(1, 1), QPointF(5, 5), Qt::black);
    QVERIFY(doc.isDirty());

    QVERIFY(doc.loadFile(path));
    QCOMPARE(doc.imageSize(), QSize(16, 16));
    QCOMPARE(doc.image().pixelColor(8, 8), QColor(Qt::red));
    QVERIFY(!doc.isDirty());
    QVERIFY(!doc.canUndo());
    QCOMPARE(doc.filePath(), path);
}

void TestDocument::fileNameFollowsPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Document doc;
    QCOMPARE(doc.fileName(), QString());

    const QString path = dir.filePath("picture.png");
    QVERIFY(doc.saveFile(path));
    QCOMPARE(doc.fileName(), QStringLiteral("picture.png"));
}

QTEST_MAIN(TestDocument)
#include "tst_document.moc"
