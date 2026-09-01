import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import OmaPaint

ApplicationWindow {
    id: window

    property url startupFile
    property size startupSize: Qt.size(1280, 720)
    property var pendingAction: null

    width: 1360
    height: 860
    visible: true
    title: (doc.dirty ? "*" : "")
           + (doc.fileName.length > 0 ? doc.fileName : qsTr("Untitled"))
           + " — OmaPaint"

    Document {
        id: doc
    }

    Component.onCompleted: {
        if (startupFile.toString().length > 0) {
            if (!doc.load(startupFile))
                errorDialog.show(qsTr("Could not open %1: %2")
                                 .arg(startupFile.toString()).arg(doc.lastError))
        } else {
            doc.newDocument(startupSize.width, startupSize.height)
        }
    }

    // Runs action immediately when the document is clean, otherwise asks first.
    function confirmDiscard(action) {
        if (!doc.dirty) {
            action()
            return
        }
        pendingAction = action
        discardDialog.open()
    }

    function requestNew() {
        confirmDiscard(() => doc.newDocument(startupSize.width, startupSize.height))
    }

    function requestOpen() {
        confirmDiscard(() => openDialog.open())
    }

    function requestSave() {
        if (doc.filePath.length > 0) {
            if (!doc.save())
                errorDialog.show(qsTr("Could not save: %1").arg(doc.lastError))
        } else {
            saveDialog.open()
        }
    }

    onClosing: (close) => {
        if (doc.dirty) {
            close.accepted = false
            confirmDiscard(() => Qt.quit())
        }
    }

    header: EditorToolBar {
        doc: doc
        canvas: canvas
        onNewRequested: window.requestNew()
        onOpenRequested: window.requestOpen()
        onSaveRequested: window.requestSave()
        onSaveAsRequested: saveDialog.open()
        onColorRequested: colorDialog.open()
    }

    Flickable {
        anchors.fill: parent
        contentWidth: Math.max(width, canvas.width)
        contentHeight: Math.max(height, canvas.height)
        clip: true
        ScrollBar.vertical: ScrollBar {}
        ScrollBar.horizontal: ScrollBar {}

        CanvasItem {
            id: canvas
            document: doc
            x: Math.max(0, (parent.width - width) / 2)
            y: Math.max(0, (parent.height - height) / 2)
        }
    }

    FileDialog {
        id: openDialog
        nameFilters: [qsTr("PNG images (*.png)"), qsTr("All files (*)")]
        onAccepted: {
            if (!doc.load(selectedFile))
                errorDialog.show(qsTr("Could not open: %1").arg(doc.lastError))
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        nameFilters: [qsTr("PNG images (*.png)")]
        onAccepted: {
            if (!doc.saveAs(selectedFile))
                errorDialog.show(qsTr("Could not save: %1").arg(doc.lastError))
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: canvas.foregroundColor
        onAccepted: canvas.foregroundColor = selectedColor
    }

    MessageDialog {
        id: discardDialog
        title: qsTr("Unsaved changes")
        text: qsTr("The image has unsaved changes.")
        informativeText: qsTr("Discard them?")
        buttons: MessageDialog.Discard | MessageDialog.Cancel
        onButtonClicked: (button, role) => {
            if (button === MessageDialog.Discard && window.pendingAction) {
                const action = window.pendingAction
                window.pendingAction = null
                action()
            }
        }
    }

    MessageDialog {
        id: errorDialog
        title: qsTr("Error")
        buttons: MessageDialog.Ok

        function show(message) {
            text = message
            open()
        }
    }

    Shortcut { sequences: [StandardKey.New]; onActivated: window.requestNew() }
    Shortcut { sequences: [StandardKey.Open]; onActivated: window.requestOpen() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: window.requestSave() }
    Shortcut { sequences: [StandardKey.SaveAs]; onActivated: saveDialog.open() }
    Shortcut { sequences: [StandardKey.Undo]; onActivated: doc.undo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: doc.redo() }
    Shortcut { sequence: "P"; onActivated: canvas.tool = CanvasItem.Pencil }
    Shortcut { sequence: "E"; onActivated: canvas.tool = CanvasItem.Eraser }
}
