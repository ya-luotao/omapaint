#pragma once

#include <QImage>
#include <QString>

// Thin wrappers around Qt's image readers/writers.
// All images are normalized to ARGB32_Premultiplied on load so the rest of
// the application can assume a single pixel format.
namespace ImageIo {

bool load(const QString &path, QImage *out, QString *error = nullptr);
bool save(const QString &path, const QImage &image, QString *error = nullptr);

} // namespace ImageIo
