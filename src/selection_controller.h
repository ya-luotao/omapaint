#pragma once

#include <QImage>
#include <QPointF>
#include <QRect>

class Document;

// The floating-selection state machine — PLAN.md's flagged hard part of the
// Edit Images milestone.
//
//   None ──press+drag──▶ Selecting ──release──▶ Selected (marquee, pixels
//   still in the image) ──press inside──▶ Floating (pixels lifted into an
//   uncommitted layer that follows the pointer)
//
// Classic Paint semantics: pressing outside, switching tools, or performing
// any other operation commits the floating layer. The whole
// lift → move → commit cycle collapses into ONE undo command (before = the
// pre-lift snapshot of the union region, after = the final pixels), so a
// single undo restores everything.
class SelectionController
{
public:
    enum class State {
        None,
        Selecting, // marquee being dragged out
        Selected,  // marquee finished, pixels untouched
        Floating,  // uncommitted pixel layer
    };

    explicit SelectionController(Document *document = nullptr);

    void setDocument(Document *document); // also resets all state
    void reset();                         // drop everything, no commit

    State state() const { return m_state; }
    bool hasSelection() const;
    bool isFloating() const { return m_state == State::Floating; }
    QRect selectionRect() const; // marquee or floating bounds, image coords
    QImage floatingImage() const { return m_float; }
    QPointF floatingPos() const { return m_floatPos; }

    // Pointer interface, image coordinates.
    void pointerPress(const QPointF &pos);
    void pointerMove(const QPointF &pos);
    void pointerRelease(const QPointF &pos);

    // Operations. Any of these acting on a floating layer commits it first
    // (except cancelFloating, which is the undo-while-floating gesture).
    void selectAll();
    void deselect();       // commit + clear marquee
    void commit();         // composite the floating layer, push one command
    void cancelFloating(); // move-lift: restore source pixels; paste: discard
    QImage copyImage() const;             // selection or floating content
    QImage takeCut();                     // copy + clear region (one command)
    bool deleteSelection();               // clear region (one command)
    void paste(const QImage &image, const QPointF &center);
    QRect cropRect() const;               // valid only for a usable marquee

private:
    void liftSelection(const QPointF &grabPos);
    QRect floatingRect() const;
    void clearRegion(const QRect &rect);

    Document *m_document = nullptr;
    State m_state = State::None;

    QPoint m_anchor;
    QRect m_rect; // marquee, normalized, clamped to the image

    QImage m_float;
    QPointF m_floatPos;
    QPointF m_dragOffset;
    bool m_dragging = false;

    QImage m_liftSnapshot; // image as it was before the lift/paste (CoW)
    QRect m_sourceRect;    // cleared region for a move-lift; null for paste
    bool m_fromPaste = false;
};
