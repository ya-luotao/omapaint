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

        // Foreground over background, classic Paint style. Click the front
        // swatch for the color dialog; X swaps them.
        AbstractButton {
            implicitWidth: 40
            implicitHeight: 40
            onClicked: root.customColorRequested()

            contentItem: Item {
                Rectangle {
                    x: 12; y: 12
                    width: 24; height: 24
                    color: root.canvas.backgroundColor
                    border.color: root.palette.mid
                    border.width: 1
                    radius: 3
                }
                Rectangle {
                    width: 24; height: 24
                    color: root.canvas.foregroundColor
                    border.color: root.palette.mid
                    border.width: 1
                    radius: 3
                }
            }

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Foreground / background — click for custom, X swaps, right-click a swatch sets background")
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

                    // Right click sets the background color.
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: root.canvas.backgroundColor = parent.modelData
                    }

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
