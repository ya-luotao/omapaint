#pragma once

#include <QImage>
#include <QUndoCommand>

class Document;

// Whole-image operations (crop, image resize, canvas resize) store full
// before/after snapshots, as PLAN.md allows for size-changing edits.
class ImageCommand : public QUndoCommand
{
public:
    ImageCommand(Document *document, const QImage &before, const QImage &after,
                 const QString &text);

    void undo() override;
    void redo() override;

private:
    Document *m_document;
    QImage m_before;
    QImage m_after;
};
