#include "commands/draw_command.h"
#include "document.h"
#include "ruby.h"

#include <QtTest>

class TestRuby : public QObject
{
    Q_OBJECT

private slots:
    void sameSeedSameWalk();
    void damageCoversAllInk();
    void pawPrintsUndoRedo();
    void portraitUsesGivenColors();
};

void TestRuby::sameSeedSameWalk()
{
    QImage a(320, 200, QImage::Format_ARGB32_Premultiplied);
    a.fill(Qt::white);
    QImage b = a;

    const QRect da = Ruby::stampPawPrints(a, Qt::black, 7);
    const QRect db = Ruby::stampPawPrints(b, Qt::black, 7);
    QCOMPARE(a, b);
    QCOMPARE(da, db);
    QVERIFY(!da.isEmpty());

    // A different seed wanders differently.
    QImage c(320, 200, QImage::Format_ARGB32_Premultiplied);
    c.fill(Qt::white);
    Ruby::stampPawPrints(c, Qt::black, 8);
    QVERIFY(c != a);
}

void TestRuby::damageCoversAllInk()
{
    QImage image(400, 300, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    const QImage before = image;
    const QRect damage = Ruby::stampPawPrints(image, Qt::red, 42);

    // Every changed pixel must be inside the reported damage rect.
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixel(x, y) != before.pixel(x, y))
                QVERIFY2(damage.contains(x, y),
                         qPrintable(QStringLiteral("(%1,%2) outside %3,%4 %5x%6")
                                        .arg(x).arg(y).arg(damage.x())
                                        .arg(damage.y()).arg(damage.width())
                                        .arg(damage.height())));
}

// Mirrors CanvasItem::rubyWalk: stamp, then push one region-based command.
void TestRuby::pawPrintsUndoRedo()
{
    Document doc;
    doc.newDocument(320, 240);
    const QImage original = doc.image();

    const QImage before = doc.image();
    const QRect damage = Ruby::stampPawPrints(doc.image(), QColor("#c34043"), 3);
    doc.undoStack()->push(
        new DrawCommand(&doc, damage, before.copy(damage),
                        doc.image().copy(damage), "Ruby's paw prints"));
    const QImage walked = doc.image();
    QVERIFY(walked != original);
    QVERIFY(doc.isDirty());

    doc.undo();
    QCOMPARE(doc.image(), original);
    doc.redo();
    QCOMPARE(doc.image(), walked);
}

void TestRuby::portraitUsesGivenColors()
{
    const QColor fur("#112233"), accent("#c34043");
    const QImage portrait = Ruby::portrait(fur, accent, 1);
    QVERIFY(!portrait.isNull());

    bool sawFur = false, sawAccent = false, sawOther = false;
    for (int y = 0; y < portrait.height(); ++y) {
        for (int x = 0; x < portrait.width(); ++x) {
            const QColor c = portrait.pixelColor(x, y);
            if (c.alpha() == 0)
                continue;
            if (c == fur)
                sawFur = true;
            else if (c == accent)
                sawAccent = true;
            else
                sawOther = true;
        }
    }
    QVERIFY(sawFur);
    QVERIFY(sawAccent);
    QVERIFY(!sawOther);
}

QTEST_MAIN(TestRuby)
#include "tst_ruby.moc"
