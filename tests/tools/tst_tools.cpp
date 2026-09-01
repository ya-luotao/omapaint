#include "tools/brush_tool.h"
#include "tools/ellipse_tool.h"
#include "tools/eraser_tool.h"
#include "tools/fill_tool.h"
#include "tools/line_tool.h"
#include "tools/pencil_tool.h"
#include "tools/rectangle_tool.h"

#include <QtTest>

class TestTools : public QObject
{
    Q_OBJECT

private slots:
    void pencilDrawsHorizontalLine();
    void pencilSinglePointDraws();
    void pencilIgnoresBrushSize();
    void brushRespectsBrushSize();
    void eraserClearsToTransparent();
    void damageRectCoversStroke();
    void damageRectClampedToImage();

    void linePreviewRestoresBackground();
    void rectanglePreviewMatchesDirectDraw();
    void ellipseDrawsWithinBounds();

    void fillFloodsEnclosedRegion();
    void fillSameColorIsNoOp();
    void fillOutsideImageIsNoOp();
    void fillDamageCoversChangedPixels();
};

static QImage whiteImage(int w, int h)
{
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    return image;
}

static ToolContext ctx(const QColor &color, qreal size = 1)
{
    return ToolContext{color, size};
}

void TestTools::pencilDrawsHorizontalLine()
{
    QImage image = whiteImage(40, 20);
    PencilTool pencil;

    pencil.begin(image, QPointF(5, 10), ctx(Qt::black));
    pencil.move(image, QPointF(30, 10), ctx(Qt::black));
    pencil.end(image, QPointF(30, 10), ctx(Qt::black));

    for (int x = 6; x <= 29; ++x)
        QCOMPARE(image.pixelColor(x, 10), QColor(Qt::black));
    QCOMPARE(image.pixelColor(10, 5), QColor(Qt::white));
    QCOMPARE(image.pixelColor(10, 15), QColor(Qt::white));
}

void TestTools::pencilSinglePointDraws()
{
    QImage image = whiteImage(10, 10);
    PencilTool pencil;

    pencil.begin(image, QPointF(4, 4), ctx(Qt::red));

    QCOMPARE(image.pixelColor(4, 4), QColor(Qt::red));
}

void TestTools::pencilIgnoresBrushSize()
{
    QImage image = whiteImage(20, 20);
    PencilTool pencil;

    pencil.begin(image, QPointF(5, 10), ctx(Qt::black, 10));
    pencil.move(image, QPointF(15, 10), ctx(Qt::black, 10));

    QCOMPARE(image.pixelColor(10, 10), QColor(Qt::black));
    QCOMPARE(image.pixelColor(10, 13), QColor(Qt::white)); // still 1px wide
}

void TestTools::brushRespectsBrushSize()
{
    QImage image = whiteImage(40, 40);
    BrushTool brush;

    brush.begin(image, QPointF(10, 20), ctx(Qt::black, 9));
    brush.move(image, QPointF(30, 20), ctx(Qt::black, 9));

    QCOMPARE(image.pixelColor(20, 20), QColor(Qt::black));
    QCOMPARE(image.pixelColor(20, 17), QColor(Qt::black)); // within half width
    QCOMPARE(image.pixelColor(20, 23), QColor(Qt::black));
    QCOMPARE(image.pixelColor(20, 10), QColor(Qt::white)); // outside
}

void TestTools::eraserClearsToTransparent()
{
    QImage image = whiteImage(60, 60);
    EraserTool eraser;

    eraser.begin(image, QPointF(30, 30), ctx(Qt::black, 16));

    QCOMPARE(image.pixelColor(30, 30).alpha(), 0);
    QCOMPARE(image.pixelColor(5, 5), QColor(Qt::white));
}

void TestTools::damageRectCoversStroke()
{
    QImage image = whiteImage(100, 100);
    BrushTool brush;

    QRect damage = brush.begin(image, QPointF(20, 20), ctx(Qt::black, 7));
    damage |= brush.move(image, QPointF(60, 40), ctx(Qt::black, 7));

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != QColor(Qt::white))
                QVERIFY2(damage.contains(x, y),
                         qPrintable(QStringLiteral("pixel (%1,%2) outside damage")
                                        .arg(x).arg(y)));
        }
    }
}

void TestTools::damageRectClampedToImage()
{
    QImage image = whiteImage(30, 30);
    EraserTool eraser;

    QRect damage = eraser.begin(image, QPointF(1, 1), ctx(Qt::black, 16));
    damage |= eraser.move(image, QPointF(29, 29), ctx(Qt::black, 16));

    QVERIFY(image.rect().contains(damage));
}

// The core rubber-band invariant: after any number of preview moves, the
// image must equal drawing the final shape directly once.
void TestTools::linePreviewRestoresBackground()
{
    QImage direct = whiteImage(80, 60);
    QImage previewed = direct;

    LineTool tool;
    const ToolContext c = ctx(Qt::blue, 3);

    tool.begin(previewed, QPointF(10, 10), c);
    tool.move(previewed, QPointF(70, 50), c);
    tool.move(previewed, QPointF(20, 45), c);
    tool.move(previewed, QPointF(60, 15), c);
    tool.end(previewed, QPointF(60, 15), c);

    LineTool reference;
    reference.begin(direct, QPointF(10, 10), c);
    reference.move(direct, QPointF(60, 15), c);
    reference.end(direct, QPointF(60, 15), c);

    QCOMPARE(previewed, direct);
}

void TestTools::rectanglePreviewMatchesDirectDraw()
{
    QImage direct = whiteImage(80, 60);
    QImage previewed = direct;

    RectangleTool tool;
    const ToolContext c = ctx(Qt::red, 1);

    tool.begin(previewed, QPointF(15, 12), c);
    tool.move(previewed, QPointF(70, 55), c);
    tool.move(previewed, QPointF(40, 30), c);
    tool.end(previewed, QPointF(40, 30), c);

    RectangleTool reference;
    reference.begin(direct, QPointF(15, 12), c);
    reference.move(direct, QPointF(40, 30), c);
    reference.end(direct, QPointF(40, 30), c);

    QCOMPARE(previewed, direct);

    // Outline drawn, interior untouched.
    QCOMPARE(previewed.pixelColor(15, 12), QColor(Qt::red));
    QCOMPARE(previewed.pixelColor(25, 20), QColor(Qt::white));
}

void TestTools::ellipseDrawsWithinBounds()
{
    QImage image = whiteImage(60, 60);
    EllipseTool tool;
    const ToolContext c = ctx(Qt::black, 1);

    QRect damage = tool.begin(image, QPointF(10, 10), c);
    damage |= tool.move(image, QPointF(50, 40), c);
    tool.end(image, QPointF(50, 40), c);

    // Something was drawn, all of it inside the damage rect.
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
    QCOMPARE(image.pixelColor(30, 25), QColor(Qt::white)); // interior empty
}

void TestTools::fillFloodsEnclosedRegion()
{
    QImage image = whiteImage(40, 40);
    // Enclose a region with a black rectangle outline.
    {
        QPainter painter(&image);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRect(10, 10, 20, 20);
    }

    FillTool fill;
    fill.begin(image, QPointF(20, 20), ctx(Qt::green));

    QCOMPARE(image.pixelColor(20, 20), QColor(Qt::green));
    QCOMPARE(image.pixelColor(11, 11), QColor(Qt::green)); // inside corner
    QCOMPARE(image.pixelColor(5, 5), QColor(Qt::white));   // outside untouched
    QCOMPARE(image.pixelColor(10, 10), QColor(Qt::black)); // border untouched
}

void TestTools::fillSameColorIsNoOp()
{
    QImage image = whiteImage(20, 20);
    FillTool fill;

    const QRect damage = fill.begin(image, QPointF(10, 10), ctx(Qt::white));

    QVERIFY(damage.isEmpty());
    QCOMPARE(image, whiteImage(20, 20));
}

void TestTools::fillOutsideImageIsNoOp()
{
    QImage image = whiteImage(20, 20);
    FillTool fill;

    QVERIFY(fill.begin(image, QPointF(-5, 40), ctx(Qt::red)).isEmpty());
    QCOMPARE(image, whiteImage(20, 20));
}

void TestTools::fillDamageCoversChangedPixels()
{
    QImage image = whiteImage(50, 50);
    FillTool fill;

    const QRect damage = fill.begin(image, QPointF(25, 25), ctx(Qt::blue));

    QCOMPARE(damage, image.rect()); // whole white canvas flooded
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::blue));
    QCOMPARE(image.pixelColor(49, 49), QColor(Qt::blue));
}

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
