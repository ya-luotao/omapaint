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
    signal colorRequested()

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

        ToolButton {
            text: qsTr("Pencil")
            checkable: true
            checked: root.canvas.tool === CanvasItem.Pencil
            onClicked: root.canvas.tool = CanvasItem.Pencil
        }
        ToolButton {
            text: qsTr("Eraser")
            checkable: true
            checked: root.canvas.tool === CanvasItem.Eraser
            onClicked: root.canvas.tool = CanvasItem.Eraser
        }

        ToolSeparator {}

        // Foreground color swatch.
        AbstractButton {
            implicitWidth: 28
            implicitHeight: 28
            onClicked: root.colorRequested()

            contentItem: Rectangle {
                color: root.canvas.foregroundColor
                border.color: root.palette.mid
                border.width: 1
                radius: 3
            }
        }

        Item { Layout.fillWidth: true }
    }
}
