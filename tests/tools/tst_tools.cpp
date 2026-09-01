#include "tools/eraser_tool.h"
#include "tools/pencil_tool.h"

#include <QtTest>

class TestTools : public QObject
{
    Q_OBJECT

private slots:
    void pencilDrawsHorizontalLine();
    void pencilSinglePointDraws();
    void eraserClearsToTransparent();
    void damageRectCoversStroke();
    void damageRectClampedToImage();
};

static QImage whiteImage(int w, int h)
{
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    return image;
}

void TestTools::pencilDrawsHorizontalLine()
{
    QImage image = whiteImage(40, 20);
    PencilTool pencil;

    pencil.begin(image, QPointF(5, 10), Qt::black);
    pencil.move(image, QPointF(30, 10), Qt::black);
    pencil.end();

    for (int x = 6; x <= 29; ++x)
        QCOMPARE(image.pixelColor(x, 10), QColor(Qt::black));
    QCOMPARE(image.pixelColor(10, 5), QColor(Qt::white));
    QCOMPARE(image.pixelColor(10, 15), QColor(Qt::white));
}

void TestTools::pencilSinglePointDraws()
{
    QImage image = whiteImage(10, 10);
    PencilTool pencil;

    pencil.begin(image, QPointF(4, 4), Qt::red);
    pencil.end();

    QCOMPARE(image.pixelColor(4, 4), QColor(Qt::red));
}

void TestTools::eraserClearsToTransparent()
{
    QImage image = whiteImage(60, 60);
    EraserTool eraser;

    eraser.begin(image, QPointF(30, 30), Qt::black);
    eraser.end();

    QCOMPARE(image.pixelColor(30, 30).alpha(), 0);
    QCOMPARE(image.pixelColor(5, 5), QColor(Qt::white));
}

void TestTools::damageRectCoversStroke()
{
    QImage image = whiteImage(100, 100);
    PencilTool pencil;

    QRect damage = pencil.begin(image, QPointF(20, 20), Qt::black);
    damage |= pencil.move(image, QPointF(60, 40), Qt::black);
    pencil.end();

    // Every non-white pixel must lie inside the reported damage rect.
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
    EraserTool eraser; // wide pen, near the edge

    QRect damage = eraser.begin(image, QPointF(1, 1), Qt::black);
    damage |= eraser.move(image, QPointF(29, 29), Qt::black);
    eraser.end();

    QVERIFY(image.rect().contains(damage));
}

QTEST_MAIN(TestTools)
#include "tst_tools.moc"
