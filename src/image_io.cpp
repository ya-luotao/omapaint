#include "image_io.h"

#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>

namespace ImageIo {

bool load(const QString &path, QImage *out, QString *error)
{
    QImageReader reader(path);
    QImage image = reader.read();
    if (image.isNull()) {
        if (error)
            *error = reader.errorString();
        return false;
    }
    *out = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    return true;
}

bool save(const QString &path, const QImage &image, QString *error)
{
    // Resolve the format from the suffix ourselves: QImageWriter::format()
    // stays empty when only a file name was given, so it cannot be queried
    // to make decisions before writing.
    QByteArray format = QFileInfo(path).suffix().toLower().toUtf8();
    if (format.isEmpty())
        format = "png";
    QImageWriter writer(path, format);
    // Qt's webp plugin encodes losslessly at quality 100; a paint program
    // should not silently degrade saved drawings.
    if (format == "webp")
        writer.setQuality(100);

    QImage out = image;
    // JPEG has no alpha; composite onto white rather than letting the
    // encoder flatten onto black.
    if (format == "jpg" || format == "jpeg") {
        QImage flat(image.size(), QImage::Format_RGB32);
        flat.fill(Qt::white);
        QPainter painter(&flat);
        painter.drawImage(0, 0, image);
        painter.end();
        out = flat;
    }

    if (!writer.write(out)) {
        if (error)
            *error = writer.errorString();
        return false;
    }
    return true;
}

} // namespace ImageIo
