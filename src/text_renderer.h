#pragma once

#include <QColor>
#include <QFont>
#include <QImage>
#include <QRect>
#include <QString>

// Renders committed text into the image and returns the damaged rect
// (null for empty text). Free function so the text tool's output is
// testable without a QML scene.
namespace TextRenderer {

QRect render(QImage &image, const QPointF &pos, const QString &text,
             const QFont &font, const QColor &color);

} // namespace TextRenderer
