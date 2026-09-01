import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

Frame {
    id: root

    required property Document doc
    required property CanvasItem canvas

    padding: 2
    leftPadding: 8
    rightPadding: 8

    RowLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            text: root.canvas.hoverValid
                  ? qsTr("%1, %2").arg(root.canvas.hoverImagePos.x)
                                  .arg(root.canvas.hoverImagePos.y)
                  : ""
            Layout.preferredWidth: 90
        }

        Label {
            text: qsTr("%1 × %2 px").arg(root.doc.imageSize.width)
                                    .arg(root.doc.imageSize.height)
        }

        Label {
            text: qsTr("Zoom: %1%").arg(Math.round(root.canvas.zoom * 100))
        }

        Label {
            visible: root.canvas.hasSelection
            text: qsTr("Selection: %1 × %2")
                  .arg(root.canvas.selectionRect.width)
                  .arg(root.canvas.selectionRect.height)
        }

        Item { Layout.fillWidth: true }
    }
}
