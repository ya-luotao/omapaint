#include "draw_command.h"

#include "document.h"

DrawCommand::DrawCommand(Document *document, const QRect &rect,
                         const QImage &before, const QImage &after,
                         const QString &text)
    : m_document(document)
    , m_rect(rect)
    , m_before(before)
    , m_after(after)
{
    setText(text);
}

void DrawCommand::undo()
{
    m_document->applyRegion(m_before, m_rect.topLeft());
}

void DrawCommand::redo()
{
    m_document->applyRegion(m_after, m_rect.topLeft());
}
