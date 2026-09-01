#include "document.h"

#include "image_io.h"

#include <QFileInfo>
#include <QPainter>

Document::Document(QObject *parent)
    : QObject(parent)
{
    connect(&m_undoStack, &QUndoStack::cleanChanged, this, &Document::dirtyChanged);
    connect(&m_undoStack, &QUndoStack::canUndoChanged, this, &Document::undoChanged);
    connect(&m_undoStack, &QUndoStack::canRedoChanged, this, &Document::undoChanged);
    newDocument(1280, 720);
}

bool Document::isDirty() const
{
    return !m_undoStack.isClean();
}

QString Document::fileName() const
{
    return m_filePath.isEmpty() ? QString() : QFileInfo(m_filePath).fileName();
}

bool Document::canUndo() const
{
    return m_undoStack.canUndo();
}

bool Document::canRedo() const
{
    return m_undoStack.canRedo();
}

void Document::newDocument(int width, int height)
{
    m_image = QImage(qMax(1, width), qMax(1, height), QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::white);
    m_undoStack.clear();
    m_undoStack.setClean();
    setFilePath(QString());
    emit imageChanged();
}

bool Document::load(const QUrl &url)
{
    return loadFile(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

bool Document::save()
{
    if (m_filePath.isEmpty())
        return false;
    return saveFile(m_filePath);
}

bool Document::saveAs(const QUrl &url)
{
    return saveFile(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

void Document::undo()
{
    m_undoStack.undo();
}

void Document::redo()
{
    m_undoStack.redo();
}

bool Document::loadFile(const QString &path)
{
    QImage image;
    QString error;
    if (!ImageIo::load(path, &image, &error)) {
        setLastError(error);
        return false;
    }
    m_image = image;
    m_undoStack.clear();
    m_undoStack.setClean();
    setFilePath(path);
    emit imageChanged();
    return true;
}

bool Document::saveFile(const QString &path)
{
    QString error;
    if (!ImageIo::save(path, m_image, &error)) {
        setLastError(error);
        return false;
    }
    m_undoStack.setClean();
    setFilePath(path);
    return true;
}

void Document::applyRegion(const QImage &region, const QPoint &topLeft)
{
    QPainter painter(&m_image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(topLeft, region);
    painter.end();
    emit regionChanged(QRect(topLeft, region.size()));
}

void Document::setFilePath(const QString &path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    emit filePathChanged();
}

void Document::setLastError(const QString &error)
{
    m_lastError = error;
    emit lastErrorChanged();
}
