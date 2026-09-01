#include "image_io.h"

#include <QImageWriter>
#include <QTemporaryDir>
#include <QtTest>

class TestImageIo : public QObject
{
    Q_OBJECT

private slots:
    void pngRoundTripPreservesPixels();
    void pngRoundTripPreservesAlpha();
    void loadNormalizesFormat();
    void jpegSaveFlattensAlphaOntoWhite();
    void jpegRoundTripStaysOpaqueAndSized();
    void webpRoundTripPreservesPixels();
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

void TestImageIo::jpegSaveFlattensAlphaOntoWhite()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("flat.jpg");

    QImage original(10, 10, QImage::Format_ARGB32_Premultiplied);
    original.fill(Qt::transparent);
    original.setPixelColor(5, 5, QColor(255, 0, 0));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    // Transparent areas must come back white, not black.
    const QColor corner = loaded.pixelColor(0, 0);
    QVERIFY(corner.red() > 240 && corner.green() > 240 && corner.blue() > 240);
}

void TestImageIo::jpegRoundTripStaysOpaqueAndSized()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("rt.jpeg");

    QImage original(33, 21, QImage::Format_ARGB32_Premultiplied);
    original.fill(QColor(10, 200, 60));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded.size(), original.size());
    // Lossy: no per-pixel comparison, but it must be opaque and roughly green.
    QVERIFY(loaded.pixelColor(16, 10).alpha() == 255);
    QVERIFY(loaded.pixelColor(16, 10).green() > 150);
}

void TestImageIo::webpRoundTripPreservesPixels()
{
    if (!QImageWriter::supportedImageFormats().contains("webp"))
        QSKIP("webp plugin (qt6-imageformats) not available");

    QTemporaryDir dir;
    const QString path = dir.filePath("rt.webp");

    QImage original(20, 10, QImage::Format_ARGB32_Premultiplied);
    original.fill(Qt::transparent);
    original.setPixelColor(3, 4, QColor(10, 20, 30));

    QVERIFY(ImageIo::save(path, original));

    QImage loaded;
    QVERIFY(ImageIo::load(path, &loaded));
    QCOMPARE(loaded, original); // saved at quality 100 = lossless webp
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
