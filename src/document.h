#pragma once

#include <QImage>
#include <QObject>
#include <QUndoStack>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// The one open image: pixels, file path, dirty state, and undo history.
// Dirty state is derived from the undo stack's clean marker, so undoing back
// to the last save point automatically clears it.
class Document : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY filePathChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit Document(QObject *parent = nullptr);

    QImage &image() { return m_image; }
    const QImage &image() const { return m_image; }

    bool isDirty() const;
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool canUndo() const;
    bool canRedo() const;
    QSize imageSize() const { return m_image.size(); }
    QString lastError() const { return m_lastError; }

    QUndoStack *undoStack() { return &m_undoStack; }

    Q_INVOKABLE void newDocument(int width, int height);
    Q_INVOKABLE bool load(const QUrl &url);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QUrl &url);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Whole-image edits; each pushes one full-snapshot ImageCommand.
    Q_INVOKABLE void resizeImage(int width, int height);
    Q_INVOKABLE void resizeCanvas(int width, int height);
    void cropTo(const QRect &rect);

    bool loadFile(const QString &path);
    bool saveFile(const QString &path);

    // Overwrites (CompositionMode_Source, alpha included) the given region.
    // Used by undo commands to restore before/after pixels.
    void applyRegion(const QImage &region, const QPoint &topLeft);

    // Replaces the whole image (used by ImageCommand undo/redo).
    void setImage(const QImage &image);

    // For collaborators (selection controller) that paint into image()
    // directly and need the views refreshed.
    void notifyRegionChanged(const QRect &rect) { emit regionChanged(rect); }

signals:
    void dirtyChanged();
    void filePathChanged();
    void undoChanged();
    void imageChanged();                   // whole image replaced or resized
    void regionChanged(const QRect &rect); // damaged area within the image
    void lastErrorChanged();

private:
    void setFilePath(const QString &path);
    void setLastError(const QString &error);

    QImage m_image;
    QString m_filePath;
    QString m_lastError;
    QUndoStack m_undoStack;
};
