#include "text_renderer.h"
#include "tools/arrow_tool.h"
#include "tools/ellipse_tool.h"
#include "tools/pixelate_tool.h"
#include "tools/rectangle_tool.h"

#include <QPainter>
#include <QtTest>

// v0.0.4 annotation features: arrow, filled shapes, pixelate, text rendering.
class TestAnnotate : public QObject
{
    Q_OBJECT

private slots:
    void arrowInkStaysWithinDamage();
    void arrowPreviewRestoresBackground();
    void rectangleFilledUsesBackgroundColor();
    void rectangleSolidUsesForeground();
    void ellipseFilledInterior();
    void pixelateMakesBlocksAndCleansPreview();
    void pixelateOutsideRegionUntouched();
    void textRendersInkWithinDamage();
    void emptyTextIsNoOp();
    void multilineTextTallerThanSingleLine();
};

static QImage whiteImage(int w, int h)
{
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    return image;
}

void TestAnnotate::arrowInkStaysWithinDamage()
{
    QImage image = whiteImage(100, 100);
    ArrowTool arrow;
    ToolContext ctx;
    ctx.color = Qt::red;
    ctx.size = 3;

    QRect damage = arrow.begin(image, QPointF(10, 50), ctx);
    damage |= arrow.move(image, QPointF(80, 50), ctx);
    arrow.end(image, QPointF(80, 50), ctx);

    bool anyInk = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != QColor(Qt::white)) {
                anyInk = true;
                QVERIFY2(damage.contains(x, y),
                         qPrintable(QStringLiteral("ink at (%1,%2) outside damage")
                                        .arg(x).arg(y)));
            }
        }
    }
    QVERIFY(anyInk);
    // Shaft drawn along the middle, head near the tip.
    QCOMPARE(image.pixelColor(40, 50), QColor(Qt::red));
    QCOMPARE(image.pixelColor(78, 50), QColor(Qt::red));
}

void TestAnnotate::arrowPreviewRestoresBackground()
{
    QImage direct = whiteImage(100, 80);
    QImage previewed = direct;
    ToolContext ctx;
    ctx.color = Qt::blue;
    ctx.size = 5;

    ArrowTool tool;
    tool.begin(previewed, QPointF(10, 10), ctx);
    tool.move(previewed, QPointF(90, 70), ctx);
    tool.move(previewed, QPointF(20, 60), ctx);
    tool.move(previewed, QPointF(80, 20), ctx);
    tool.end(previewed, QPointF(80, 20), ctx);

    ArrowTool reference;
    reference.begin(direct, QPointF(10, 10), ctx);
    reference.move(direct, QPointF(80, 20), ctx);
    reference.end(direct, QPointF(80, 20), ctx);

    QCOMPARE(previewed, direct);
}

void TestAnnotate::rectangleFilledUsesBackgroundColor()
{
    QImage image = whiteImage(60, 60);
    RectangleTool tool;
    ToolContext ctx;
    ctx.color = Qt::black;
    ctx.fillColor = Qt::yellow;
    ctx.shapeFill = ShapeFill::Filled;

    tool.begin(image, QPointF(10, 10), ctx);
    tool.move(image, QPointF(50, 40), ctx);
    tool.end(image, QPointF(50, 40), ctx);

    QCOMPARE(image.pixelColor(10, 10), QColor(Qt::black));  // outline
    QCOMPARE(image.pixelColor(30, 25), QColor(Qt::yellow)); // interior
    QCOMPARE(image.pixelColor(5, 5), QColor(Qt::white));    // outside
}

void TestAnnotate::rectangleSolidUsesForeground()
{
    QImage image = whiteImage(60, 60);
    RectangleTool tool;
    ToolContext ctx;
    ctx.color = Qt::darkGreen;
    ctx.fillColor = Qt::yellow;
    ctx.shapeFill = ShapeFill::Solid;

    tool.begin(image, QPointF(10, 10), ctx);
    tool.move(image, QPointF(40, 40), ctx);
    tool.end(image, QPointF(40, 40), ctx);

    QCOMPARE(image.pixelColor(25, 25), QColor(Qt::darkGreen));
    QCOMPARE(image.pixelColor(10, 10), QColor(Qt::darkGreen));
}

void TestAnnotate::ellipseFilledInterior()
{
    QImage image = whiteImage(60, 60);
    EllipseTool tool;
    ToolContext ctx;
    ctx.color = Qt::black;
    ctx.fillColor = Qt::cyan;
    ctx.shapeFill = ShapeFill::Filled;

    tool.begin(image, QPointF(5, 5), ctx);
    tool.move(image, QPointF(55, 55), ctx);
    tool.end(image, QPointF(55, 55), ctx);

    QCOMPARE(image.pixelColor(30, 30), QColor(Qt::cyan)); // center
    QCOMPARE(image.pixelColor(2, 2), QColor(Qt::white));  // corner outside
}

void TestAnnotate::pixelateMakesBlocksAndCleansPreview()
{
    // Noisy content so pixelation visibly changes pixels.
    QImage image = whiteImage(80, 80);
    {
        QPainter painter(&image);
        for (int i = 0; i < 40; ++i)
            painter.fillRect(10 + i, 10 + (i * 7) % 40, 3, 3,
                             QColor(i * 6 % 255, 255 - i * 5, i * 11 % 255));
    }
    const QImage original = image;

    PixelateTool tool;
    const ToolContext ctx;
    QRect damage = tool.begin(image, QPointF(10, 10), ctx);
    damage |= tool.move(image, QPointF(60, 60), ctx);
    damage |= tool.end(image, QPointF(60, 60), ctx);

    // Region changed, and within one block every pixel is identical.
    const QRect rect(10, 10, 50, 50);
    QVERIFY(image.copy(rect) != original.copy(rect));
    const QColor c = image.pixelColor(12, 12);
    QCOMPARE(image.pixelColor(13, 13), c);
    QCOMPARE(image.pixelColor(14, 12), c);

    // No dashed preview left behind: everything outside the rect matches
    // the original.
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!rect.contains(x, y))
                QCOMPARE(image.pixelColor(x, y), original.pixelColor(x, y));
        }
    }
    QVERIFY(damage.contains(rect));
}

void TestAnnotate::pixelateOutsideRegionUntouched()
{
    QImage image = whiteImage(40, 40);
    image.setPixelColor(35, 35, Qt::red);
    PixelateTool tool;
    const ToolContext ctx;

    tool.begin(image, QPointF(5, 5), ctx);
    tool.move(image, QPointF(20, 20), ctx);
    tool.end(image, QPointF(20, 20), ctx);

    QCOMPARE(image.pixelColor(35, 35), QColor(Qt::red));
}

void TestAnnotate::textRendersInkWithinDamage()
{
    QImage image = whiteImage(200, 100);
    QFont font;
    font.setPixelSize(20);

    const QRect damage = TextRenderer::render(image, QPointF(10, 10),
                                              QStringLiteral("Hi!"), font,
                                              Qt::black);
    QVERIFY(!damage.isNull());

    bool anyInk = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != QColor(Qt::white)) {
                anyInk = true;
                QVERIFY(damage.contains(x, y));
            }
        }
    }
    QVERIFY(anyInk);
}

void TestAnnotate::emptyTextIsNoOp()
{
    QImage image = whiteImage(50, 50);
    const QImage original = image;
    QFont font;

    QVERIFY(TextRenderer::render(image, QPointF(5, 5), QString(), font,
                                 Qt::black).isNull());
    QVERIFY(TextRenderer::render(image, QPointF(5, 5), QStringLiteral("  \n "),
                                 font, Qt::black).isNull());
    QCOMPARE(image, original);
}

void TestAnnotate::multilineTextTallerThanSingleLine()
{
    QImage a = whiteImage(300, 200);
    QImage b = whiteImage(300, 200);
    QFont font;
    font.setPixelSize(16);

    const QRect one = TextRenderer::render(a, QPointF(5, 5),
                                           QStringLiteral("line"), font,
                                           Qt::black);
    const QRect two = TextRenderer::render(b, QPointF(5, 5),
                                           QStringLiteral("line\nline"), font,
                                           Qt::black);
    QVERIFY(two.height() > one.height());
}

QTEST_MAIN(TestAnnotate)
#include "tst_annotate.moc"
