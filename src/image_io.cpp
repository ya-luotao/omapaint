#include "image_io.h"

#include <QImageReader>
#include <QImageWriter>

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
    QImageWriter writer(path);
    if (writer.format().isEmpty())
        writer.setFormat("png");
    if (!writer.write(image)) {
        if (error)
            *error = writer.errorString();
        return false;
    }
    return true;
}

} // namespace ImageIo
