#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>

// Thin wrappers around Qt's image readers/writers.
// All images are normalized to ARGB32_Premultiplied on load so the rest of
// the application can assume a single pixel format. EXIF orientation is
// applied on load (photos from cameras/phones arrive rotated via metadata).
namespace ImageIo {

bool load(const QString &path, QImage *out, QString *error = nullptr);
// Decodes in-memory image data (stdin piping); same normalization as load().
bool loadData(const QByteArray &data, QImage *out, QString *error = nullptr);
bool save(const QString &path, const QImage &image, QString *error = nullptr);

} // namespace ImageIo
