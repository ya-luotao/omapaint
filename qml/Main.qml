import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import OmaPaint

ApplicationWindow {
    id: window

    property url startupFile
    property size startupSize: Qt.size(1280, 720)
    property var pendingAction: null

    width: 1400
    height: 900
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
        canvas.commitSelection()
        confirmDiscard(() => doc.newDocument(startupSize.width, startupSize.height))
    }

    function requestOpen() {
        canvas.commitSelection()
        confirmDiscard(() => openDialog.open())
    }

    function requestSave() {
        canvas.commitSelection()
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
        onSaveAsRequested: { canvas.commitSelection(); saveDialog.open() }
        onResizeRequested: (mode) => resizeDialog.openFor(mode)
    }

    footer: ColumnLayout {
        spacing: 0

        ColorPalette {
            Layout.fillWidth: true
            canvas: canvas
            onCustomColorRequested: colorDialog.open()
        }

        StatusBar {
            Layout.fillWidth: true
            doc: doc
            canvas: canvas
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ToolPanel {
            Layout.fillHeight: true
            canvas: canvas
        }

        // Viewport: the Flickable only supplies scrollbars and pan state; the
        // canvas is a fixed overlay that paints the visible part of the image.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                contentWidth: doc.imageSize.width * canvas.zoom
                contentHeight: doc.imageSize.height * canvas.zoom
                ScrollBar.vertical: vbar
                ScrollBar.horizontal: hbar
            }

            CanvasItem {
                id: canvas
                anchors.fill: parent
                document: doc
                panX: flick.contentX
                panY: flick.contentY
                onPanRequest: (x, y) => {
                    flick.contentX = x
                    flick.contentY = y
                }
            }

            // Declared after the canvas so they stack above it and stay
            // clickable.
            ScrollBar {
                id: vbar
                anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
            }
            ScrollBar {
                id: hbar
                orientation: Qt.Horizontal
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            }
        }
    }

    FileDialog {
        id: openDialog
        nameFilters: [
            qsTr("Images (*.png *.jpg *.jpeg *.webp)"),
            qsTr("All files (*)"),
        ]
        onAccepted: {
            if (!doc.load(selectedFile))
                errorDialog.show(qsTr("Could not open: %1").arg(doc.lastError))
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        nameFilters: [
            qsTr("PNG images (*.png)"),
            qsTr("JPEG images (*.jpg *.jpeg)"),
            qsTr("WebP images (*.webp)"),
        ]
        onAccepted: {
            if (!doc.saveAs(selectedFile))
                errorDialog.show(qsTr("Could not save: %1").arg(doc.lastError))
        }
    }

    Dialog {
        id: resizeDialog
        property bool canvasMode: false
        title: canvasMode ? qsTr("Resize canvas") : qsTr("Resize image")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        function openFor(mode) {
            canvasMode = (mode === "canvas")
            widthSpin.value = doc.imageSize.width
            heightSpin.value = doc.imageSize.height
            open()
        }

        onAccepted: {
            canvas.commitSelection()
            if (canvasMode)
                doc.resizeCanvas(widthSpin.value, heightSpin.value)
            else
                doc.resizeImage(widthSpin.value, heightSpin.value)
        }

        RowLayout {
            SpinBox { id: widthSpin; from: 1; to: 8192; editable: true }
            Label { text: "×" }
            SpinBox { id: heightSpin; from: 1; to: 8192; editable: true }
            Label { text: qsTr("px") }
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
    // Undo/redo must go through the canvas so a pending floating selection
    // is cancelled/committed instead of history unwinding underneath it.
    Shortcut { sequences: [StandardKey.Undo]; onActivated: canvas.undo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: canvas.redo() }

    Shortcut { sequences: [StandardKey.Cut]; onActivated: canvas.cut() }
    Shortcut { sequences: [StandardKey.Copy]; onActivated: canvas.copy() }
    Shortcut { sequences: [StandardKey.Paste]; onActivated: canvas.paste() }
    Shortcut { sequences: [StandardKey.SelectAll]; onActivated: canvas.selectAll() }
    Shortcut { sequences: [StandardKey.Delete]; onActivated: canvas.deleteSelection() }
    Shortcut { sequence: "Escape"; onActivated: canvas.escape() }

    Shortcut { sequence: "P"; onActivated: canvas.tool = CanvasItem.Pencil }
    Shortcut { sequence: "B"; onActivated: canvas.tool = CanvasItem.Brush }
    Shortcut { sequence: "E"; onActivated: canvas.tool = CanvasItem.Eraser }
    Shortcut { sequence: "L"; onActivated: canvas.tool = CanvasItem.Line }
    Shortcut { sequence: "R"; onActivated: canvas.tool = CanvasItem.Rectangle }
    Shortcut { sequence: "O"; onActivated: canvas.tool = CanvasItem.Ellipse }
    Shortcut { sequence: "F"; onActivated: canvas.tool = CanvasItem.Fill }
    Shortcut { sequence: "I"; onActivated: canvas.tool = CanvasItem.Eyedropper }
    Shortcut { sequence: "S"; onActivated: canvas.tool = CanvasItem.Selection }

    Shortcut { sequence: "["; onActivated: canvas.brushSize = canvas.brushSize - 1 }
    Shortcut { sequence: "]"; onActivated: canvas.brushSize = canvas.brushSize + 1 }

    Shortcut { sequences: ["+", "="]; onActivated: canvas.zoomIn() }
    Shortcut { sequence: "-"; onActivated: canvas.zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: canvas.resetZoom() }
    Shortcut { sequence: "G"; onActivated: canvas.pixelGrid = !canvas.pixelGrid }
}
