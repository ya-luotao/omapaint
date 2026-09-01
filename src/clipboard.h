#pragma once

#include <QImage>
#include <QObject>

// Thin wrapper over the system clipboard: image get/set plus a change signal
// so the UI can enable/disable Paste.
class Clipboard : public QObject
{
    Q_OBJECT

public:
    explicit Clipboard(QObject *parent = nullptr);

    bool hasImage() const;
    QImage image() const;
    void setImage(const QImage &image);

signals:
    void changed();
};
