import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaPaint

Dialog {
    id: root

    required property CanvasItem canvas

    modal: true
    anchors.centerIn: parent
    padding: 28
    standardButtons: Dialog.Close

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Image {
            source: "qrc:/omapaint.png"
            sourceSize: Qt.size(96, 96)
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "OmaPaint"
            font.bold: true
            font.pixelSize: 26
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
        }

        Label {
            text: qsTr("Version %1").arg(Qt.application.version)
            opacity: 0.6
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            id: tagline
            property bool forRuby: false
            text: forRuby ? qsTr("Paint. For Ruby.") : qsTr("Paint. For Omarchy.")
            font.italic: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4

            TapHandler {
                onTapped: tagline.forRuby = !tagline.forRuby
            }
        }

        // Ruby. Pet her and she chirps; three quick taps and she takes a
        // stroll across your canvas (one undo step — she is a polite cat).
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 14
            implicitWidth: ruby.width
            implicitHeight: ruby.height

            Image {
                id: ruby
                // The provider paints her in the palette's text color.
                source: "image://ruby/" + String(root.palette.text).substring(1)
                width: 190
                height: 160
                smooth: false
                transformOrigin: Item.Bottom

                SequentialAnimation {
                    id: wiggle
                    NumberAnimation { target: ruby; property: "rotation"; to: -5; duration: 70 }
                    NumberAnimation { target: ruby; property: "rotation"; to: 5; duration: 110 }
                    NumberAnimation { target: ruby; property: "rotation"; to: 0; duration: 70 }
                }

                TapHandler {
                    onTapped: {
                        if (tapCount >= 3) {
                            meow.text = "!"
                            root.canvas.rubyWalk()
                            root.close()
                            return
                        }
                        wiggle.restart()
                        meow.purr()
                    }
                }
            }

            Label {
                id: meow
                property var sounds: ["mrrp?", "prrr", "mew.", "mrrow~"]
                anchors.left: ruby.right
                anchors.top: ruby.top
                anchors.leftMargin: -18
                font.pixelSize: 15
                opacity: 0.0

                function purr() {
                    text = sounds[Math.floor(Math.random() * sounds.length)]
                    opacity = 0.85
                    hush.restart()
                }

                Timer {
                    id: hush
                    interval: 900
                    onTriggered: meow.opacity = 0.0
                }
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }
        }

        Label {
            text: qsTr("The red dot has a name.")
            font.pixelSize: 12
            opacity: 0.45
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 2
        }
    }
}
