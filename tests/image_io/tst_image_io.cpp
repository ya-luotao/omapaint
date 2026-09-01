#include "image_io.h"

#include <QTemporaryDir>
#include <QtTest>

class TestImageIo : public QObject
{
    Q_OBJECT

private slots:
    void pngRoundTripPreservesPixels();
    void pngRoundTripPreservesAlpha();
    void loadNormalizesFormat();
    void loadMissingFileFails();
    void loadInvalidFileFails();
    void saveToBadPathFails();
    void tinyImageRoundTrip();
};

void TestImageIo::pngRoundTripPreservesPixels()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("rt.png");

    QImage original(20, 10, QImage::Format_ARGB32_Premultiplied);
    original.fill(Qt::white);
    original.setPixelColor(3, 4, QColor(10, 20, 30));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded, original);
}

void TestImageIo::pngRoundTripPreservesAlpha()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("alpha.png");

    QImage original(8, 8, QImage::Format_ARGB32_Premultiplied);
    original.fill(Qt::transparent);
    original.setPixelColor(2, 2, QColor(255, 0, 0, 128));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded.pixelColor(0, 0).alpha(), 0);
    QCOMPARE(loaded.pixelColor(2, 2).alpha(), 128);
}

void TestImageIo::loadNormalizesFormat()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("rgb.png");

    QImage rgb(4, 4, QImage::Format_RGB32);
    rgb.fill(Qt::green);
    QVERIFY(rgb.save(path));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded.format(), QImage::Format_ARGB32_Premultiplied);
}

void TestImageIo::loadMissingFileFails()
{
    QImage out;
    QString error;
    QVERIFY(!ImageIo::load("/nonexistent/nowhere.png", &out, &error));
    QVERIFY(!error.isEmpty());
}

void TestImageIo::loadInvalidFileFails()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("garbage.png");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not an image");
    file.close();

    QImage out;
    QString error;
    QVERIFY(!ImageIo::load(path, &out, &error));
    QVERIFY(!error.isEmpty());
}

void TestImageIo::saveToBadPathFails()
{
    QImage image(4, 4, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QString error;
    QVERIFY(!ImageIo::save("/nonexistent/dir/out.png", image, &error));
    QVERIFY(!error.isEmpty());
}

void TestImageIo::tinyImageRoundTrip()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("tiny.png");

    QImage original(1, 1, QImage::Format_ARGB32_Premultiplied);
    original.setPixelColor(0, 0, QColor(1, 2, 3));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded, original);
}

QTEST_MAIN(TestImageIo)
#include "tst_image_io.moc"
