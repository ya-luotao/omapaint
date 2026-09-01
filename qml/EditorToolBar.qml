import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

ToolBar {
    id: root

    required property Document doc
    required property CanvasItem canvas

    signal newRequested()
    signal openRequested()
    signal saveRequested()
    signal saveAsRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 4

        ToolButton {
            text: qsTr("New")
            onClicked: root.newRequested()
        }
        ToolButton {
            text: qsTr("Open")
            onClicked: root.openRequested()
        }
        ToolButton {
            text: qsTr("Save")
            onClicked: root.saveRequested()
        }
        ToolButton {
            text: qsTr("Save As")
            onClicked: root.saveAsRequested()
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Undo")
            enabled: root.doc.canUndo
            onClicked: root.doc.undo()
        }
        ToolButton {
            text: qsTr("Redo")
            enabled: root.doc.canRedo
            onClicked: root.doc.redo()
        }

        ToolSeparator {}

        Label { text: qsTr("Size") }
        SpinBox {
            from: 1
            to: 64
            value: root.canvas.brushSize
            onValueModified: root.canvas.brushSize = value
        }

        ToolSeparator {}

        ToolButton {
            text: "−"
            onClicked: root.canvas.zoomOut()
        }
        Label {
            text: Math.round(root.canvas.zoom * 100) + "%"
            horizontalAlignment: Text.AlignHCenter
            Layout.preferredWidth: 48
        }
        ToolButton {
            text: "+"
            onClicked: root.canvas.zoomIn()
        }
        ToolButton {
            text: qsTr("Grid")
            checkable: true
            checked: root.canvas.pixelGrid
            onClicked: root.canvas.pixelGrid = checked
        }

        Item { Layout.fillWidth: true }
    }
}
