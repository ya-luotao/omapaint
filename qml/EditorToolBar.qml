import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

ToolBar {
    id: root

    required property Document doc
    required property CanvasItem canvas
    property bool annotate: false

    signal doneRequested()
    signal newRequested()
    signal openRequested()
    signal saveRequested()
    signal saveAsRequested()
    signal resizeRequested(string mode)
    signal rotateRequested(int degrees)
    signal flipRequested(bool horizontal)
    signal fontRequested()
    signal aboutRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 4

        ToolButton {
            text: qsTr("Done")
            visible: root.annotate
            highlighted: true
            onClicked: root.doneRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save in place and finish (Ctrl+Enter)")
        }
        ToolSeparator { visible: root.annotate }

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
            enabled: root.doc.canUndo || root.canvas.floating
            onClicked: root.canvas.undo()
        }
        ToolButton {
            text: qsTr("Redo")
            enabled: root.doc.canRedo
            onClicked: root.canvas.redo()
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Crop")
            enabled: root.canvas.hasSelection
            onClicked: root.canvas.crop()
        }
        ToolButton {
            text: qsTr("Resize")
            onClicked: root.resizeRequested("image")
        }
        ToolButton {
            text: qsTr("Canvas")
            onClicked: root.resizeRequested("canvas")
        }
        ToolButton {
            text: qsTr("Rotate")
            onClicked: rotateMenu.open()

            Menu {
                id: rotateMenu
                y: parent.height
                MenuItem { text: qsTr("Rotate 90\u00b0 right (Ctrl+R)"); onTriggered: root.rotateRequested(90) }
                MenuItem { text: qsTr("Rotate 90\u00b0 left (Ctrl+Shift+R)"); onTriggered: root.rotateRequested(-90) }
                MenuItem { text: qsTr("Rotate 180\u00b0"); onTriggered: root.rotateRequested(180) }
                MenuSeparator {}
                MenuItem { text: qsTr("Flip horizontal"); onTriggered: root.flipRequested(true) }
                MenuItem { text: qsTr("Flip vertical"); onTriggered: root.flipRequested(false) }
            }
        }

        ToolSeparator {}

        Label { text: qsTr("Size") }
        SpinBox {
            from: 1
            to: 64
            value: root.canvas.brushSize
            onValueModified: root.canvas.brushSize = value
        }

        ComboBox {
            model: [qsTr("Outline"), qsTr("Filled"), qsTr("Solid")]
            currentIndex: root.canvas.shapeFillMode
            onActivated: (index) => root.canvas.shapeFillMode = index
            Layout.preferredWidth: 100
        }

        ToolButton {
            text: qsTr("Font")
            visible: root.canvas.tool === CanvasItem.Text
            onClicked: root.fontRequested()
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

        ToolButton {
            text: qsTr("About")
            onClicked: root.aboutRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("About OmaPaint (F1)")
        }
    }
}
