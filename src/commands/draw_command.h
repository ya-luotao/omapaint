#pragma once

#include <QImage>
#include <QRect>
#include <QUndoCommand>

class Document;

// One committed stroke (or any raster edit confined to a region).
// Stores the affected region's pixels before and after the operation; both
// undo() and redo() simply overwrite that region, so applying is idempotent
// and the initial redo() on push is harmless.
class DrawCommand : public QUndoCommand
{
public:
    DrawCommand(Document *document, const QRect &rect,
                const QImage &before, const QImage &after,
                const QString &text);

    void undo() override;
    void redo() override;

private:
    Document *m_document;
    QRect m_rect;
    QImage m_before;
    QImage m_after;
};
