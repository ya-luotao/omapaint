import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

// Fixed quick-access palette plus the current color and a custom picker.
Frame {
    id: root

    required property CanvasItem canvas

    signal customColorRequested()

    padding: 6

    RowLayout {
        anchors.fill: parent
        spacing: 8

        // Current foreground color; click for the full color dialog.
        AbstractButton {
            implicitWidth: 34
            implicitHeight: 34
            onClicked: root.customColorRequested()

            contentItem: Rectangle {
                color: root.canvas.foregroundColor
                border.color: root.palette.mid
                border.width: 1
                radius: 3
            }

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Custom color…")
        }

        GridLayout {
            columns: 8
            rowSpacing: 2
            columnSpacing: 2

            Repeater {
                model: [
                    "#000000", "#7f7f7f", "#880015", "#ed1c24",
                    "#ff7f27", "#fff200", "#22b14c", "#00a2e8",
                    "#ffffff", "#c3c3c3", "#b97a57", "#ffaec9",
                    "#ffc90e", "#efe4b0", "#b5e61d", "#3f48cc",
                ]

                delegate: AbstractButton {
                    required property string modelData
                    implicitWidth: 16
                    implicitHeight: 16
                    onClicked: root.canvas.foregroundColor = modelData

                    contentItem: Rectangle {
                        color: parent.modelData
                        border.color: root.palette.mid
                        border.width: 1
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
