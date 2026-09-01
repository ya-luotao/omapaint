#include "clipboard.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

Clipboard::Clipboard(QObject *parent)
    : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &Clipboard::changed);
}

bool Clipboard::hasImage() const
{
    const QMimeData *data = QGuiApplication::clipboard()->mimeData();
    return data && data->hasImage();
}

QImage Clipboard::image() const
{
    return QGuiApplication::clipboard()->image()
        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

void Clipboard::setImage(const QImage &image)
{
    QGuiApplication::clipboard()->setImage(image);
}
