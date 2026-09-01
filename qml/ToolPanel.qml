import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

// Vertical tool selector on the left edge, classic Paint style.
Frame {
    id: root

    required property CanvasItem canvas

    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: [
                { label: qsTr("Select"),     tool: CanvasItem.Selection },
                { label: qsTr("Pencil"),     tool: CanvasItem.Pencil },
                { label: qsTr("Brush"),      tool: CanvasItem.Brush },
                { label: qsTr("Eraser"),     tool: CanvasItem.Eraser },
                { label: qsTr("Line"),       tool: CanvasItem.Line },
                { label: qsTr("Rectangle"),  tool: CanvasItem.Rectangle },
                { label: qsTr("Ellipse"),    tool: CanvasItem.Ellipse },
                { label: qsTr("Fill"),       tool: CanvasItem.Fill },
                { label: qsTr("Eyedropper"), tool: CanvasItem.Eyedropper },
            ]

            delegate: ToolButton {
                required property var modelData
                Layout.fillWidth: true
                text: modelData.label
                checkable: true
                checked: root.canvas.tool === modelData.tool
                onClicked: root.canvas.tool = modelData.tool
            }
        }

        Item { Layout.fillHeight: true }
    }
}
