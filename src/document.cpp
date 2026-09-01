#include "document.h"

#include "commands/image_command.h"
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

void Document::newFromImage(const QImage &image)
{
    if (image.isNull())
        return;
    m_image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
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

void Document::resizeImage(int width, int height)
{
    width = qMax(1, width);
    height = qMax(1, height);
    if (QSize(width, height) == m_image.size())
        return;

    const QImage after =
        m_image.scaled(width, height, Qt::IgnoreAspectRatio,
                       Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_undoStack.push(new ImageCommand(this, m_image, after,
                                      QStringLiteral("Resize image")));
}

void Document::resizeCanvas(int width, int height)
{
    width = qMax(1, width);
    height = qMax(1, height);
    if (QSize(width, height) == m_image.size())
        return;

    // New area is white, matching classic Paint and the common "add caption
    // space to a screenshot" use.
    QImage after(width, height, QImage::Format_ARGB32_Premultiplied);
    after.fill(Qt::white);
    QPainter painter(&after);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(0, 0, m_image);
    painter.end();

    m_undoStack.push(new ImageCommand(this, m_image, after,
                                      QStringLiteral("Resize canvas")));
}

void Document::cropTo(const QRect &rect)
{
    const QRect target = rect.intersected(m_image.rect());
    if (target.isEmpty() || target == m_image.rect())
        return;

    m_undoStack.push(new ImageCommand(this, m_image, m_image.copy(target),
                                      QStringLiteral("Crop")));
}

void Document::setImage(const QImage &image)
{
    m_image = image;
    emit imageChanged();
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
