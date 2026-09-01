import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

// Two-column icon grid on the left edge, classic Paint style. Every icon was
// drawn with OmaPaint's own tools (devtools/draw_icons) and is tinted to the
// theme's text color at runtime.
Frame {
    id: root

    required property CanvasItem canvas

    padding: 4

    GridLayout {
        columns: 2
        rowSpacing: 2
        columnSpacing: 2

        Repeater {
            model: [
                { label: qsTr("Select (S)"),     icon: "select",     tool: CanvasItem.Selection },
                { label: qsTr("Pencil (P)"),     icon: "pencil",     tool: CanvasItem.Pencil },
                { label: qsTr("Brush (B)"),      icon: "brush",      tool: CanvasItem.Brush },
                { label: qsTr("Eraser (E)"),     icon: "eraser",     tool: CanvasItem.Eraser },
                { label: qsTr("Line (L)"),       icon: "line",       tool: CanvasItem.Line },
                { label: qsTr("Arrow (A)"),      icon: "arrow",      tool: CanvasItem.Arrow },
                { label: qsTr("Rectangle (R)"),  icon: "rectangle",  tool: CanvasItem.Rectangle },
                { label: qsTr("Ellipse (O)"),    icon: "ellipse",    tool: CanvasItem.Ellipse },
                { label: qsTr("Fill (F)"),       icon: "fill",       tool: CanvasItem.Fill },
                { label: qsTr("Text (T)"),       icon: "text",       tool: CanvasItem.Text },
                { label: qsTr("Pixelate"),       icon: "pixelate",   tool: CanvasItem.Pixelate },
                { label: qsTr("Eyedropper (I)"), icon: "eyedropper", tool: CanvasItem.Eyedropper },
            ]

            delegate: ToolButton {
                required property var modelData
                icon.source: "qrc:/icons/" + modelData.icon + ".png"
                icon.width: 22
                icon.height: 22
                icon.color: root.palette.buttonText
                display: AbstractButton.IconOnly
                checkable: true
                checked: root.canvas.tool === modelData.tool
                onClicked: root.canvas.tool = modelData.tool
                ToolTip.visible: hovered
                ToolTip.delay: 400
                ToolTip.text: modelData.label
            }
        }
    }
}
