#include "image_command.h"

#include "document.h"

ImageCommand::ImageCommand(Document *document, const QImage &before,
                           const QImage &after, const QString &text)
    : m_document(document)
    , m_before(before)
    , m_after(after)
{
    setText(text);
}

void ImageCommand::undo()
{
    m_document->setImage(m_before);
}

void ImageCommand::redo()
{
    m_document->setImage(m_after);
}
