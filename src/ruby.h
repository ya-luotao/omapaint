#pragma once

#include <QColor>
#include <QImage>
#include <QRect>

// Ruby is the chubby white cat the logo's red paint dot is named after.
// Her pixel portrait lives in the qrc (assets/ruby.txt, block art shared
// with the demo); this is where she comes to life outside of it.
namespace Ruby {

// Renders the portrait: `fur` for the ink cells, `accent` for her harness
// and the period after her name. Pixels are scaled up with no smoothing.
QImage portrait(const QColor &fur, const QColor &accent, int cellSize = 8);

// Easter egg: stamps a trail of paw prints wandering across the image and
// returns the damaged rect (for a region-based undo command). The same seed
// reproduces the same walk.
QRect stampPawPrints(QImage &image, const QColor &color, quint32 seed);

} // namespace Ruby
