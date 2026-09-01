#include "commands/draw_command.h"
#include "document.h"
#include "image_io.h"
#include "tools/brush_tool.h"
#include "tools/fill_tool.h"

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QtTest>

// 4K smoke tests per PLAN.md: no timing assertions (CI machines vary), just
// completion plus logged durations for eyeballing regressions.
class TestPerf : public QObject
{
    Q_OBJECT

private slots:
    void fourKBrushStroke();
    void fourKFloodFill();
    void fourKSaveLoadRoundTrip();
    void repeatedUndoRedo();
};

static Document *makeFourK()
{
    auto *doc = new Document;
    doc->newDocument(3840, 2160);
    return doc;
}

void TestPerf::fourKBrushStroke()
{
    QScopedPointer<Document> doc(makeFourK());
    BrushTool brush;
    ToolContext ctx;
    ctx.color = Qt::blue;
    ctx.size = 40;

    QElapsedTimer timer;
    timer.start();
    QRect damage = brush.begin(doc->image(), QPointF(50, 1000), ctx);
    for (int i = 1; i <= 200; ++i)
        damage |= brush.move(doc->image(),
                             QPointF(50 + i * 18, 1000 + ((i % 7) - 3) * 60), ctx);
    brush.end(doc->image(), QPointF(3650, 1000), ctx);
    qInfo("4K brush stroke (200 segments): %lld ms", timer.elapsed());

    QVERIFY(!damage.isEmpty());
}

void TestPerf::fourKFloodFill()
{
    QScopedPointer<Document> doc(makeFourK());
    FillTool fill;
    ToolContext ctx;
    ctx.color = Qt::red;

    QElapsedTimer timer;
    timer.start();
    const QRect damage = fill.begin(doc->image(), QPointF(1920, 1080), ctx);
    qInfo("4K flood fill (whole canvas): %lld ms", timer.elapsed());

    QCOMPARE(damage, doc->image().rect());
}

void TestPerf::fourKSaveLoadRoundTrip()
{
    QTemporaryDir dir;
    QScopedPointer<Document> doc(makeFourK());
    const QString path = dir.filePath("4k.png");

    QElapsedTimer timer;
    timer.start();
    QVERIFY(ImageIo::save(path, doc->image()));
    qInfo("4K PNG save: %lld ms", timer.elapsed());

    timer.restart();
    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    qInfo("4K PNG load: %lld ms", timer.elapsed());
    QCOMPARE(loaded.size(), QSize(3840, 2160));
}

void TestPerf::repeatedUndoRedo()
{
    QScopedPointer<Document> doc(makeFourK());
    BrushTool brush;
    ToolContext ctx;
    ctx.color = Qt::black;
    ctx.size = 30;

    // 50 strokes committed the way the canvas does it.
    for (int i = 0; i < 50; ++i) {
        const QImage before = doc->image();
        QRect damage = brush.begin(doc->image(), QPointF(100 + i * 30, 200), ctx);
        damage |= brush.move(doc->image(), QPointF(100 + i * 30, 1800), ctx);
        brush.end(doc->image(), QPointF(100 + i * 30, 1800), ctx);
        doc->undoStack()->push(new DrawCommand(doc.data(), damage,
                                               before.copy(damage),
                                               doc->image().copy(damage),
                                               QStringLiteral("Brush")));
    }

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 50; ++i)
        doc->undo();
    for (int i = 0; i < 50; ++i)
        doc->redo();
    qInfo("50 undos + 50 redos on 4K: %lld ms", timer.elapsed());

    QVERIFY(!doc->canRedo());
}

QTEST_MAIN(TestPerf)
#include "tst_perf.moc"
