import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import OmaPaint

ApplicationWindow {
    id: window

    property url startupFile
    property size startupSize: Qt.size(1280, 720)
    property bool startupClipboard: false
    property bool annotateMode: false
    property bool clipboardLoaded: false
    property var pendingAction: null
    // Single-letter shortcuts must stand down while any text field is live.
    readonly property bool typing: textOverlay.visible || resizeDialog.visible
                                   || aboutDialog.visible

    width: 1400
    height: 900
    visible: true
    title: (doc.dirty ? "*" : "")
           + (doc.fileName.length > 0 ? doc.fileName : qsTr("Untitled"))
           + " — OmaPaint"

    Document {
        id: doc
    }

    // Wayland only exposes the clipboard to the focused window, so the
    // --clipboard read waits for first activation instead of startup.
    onActiveChanged: {
        if (active && startupClipboard && !clipboardLoaded) {
            clipboardLoaded = true
            if (!canvas.loadFromClipboard())
                errorDialog.show(qsTr("The clipboard does not contain an image."))
        }
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

    function commitPendingEdits() {
        textOverlay.commit()
        canvas.commitSelection()
    }

    function requestNew() {
        commitPendingEdits()
        confirmDiscard(() => doc.newDocument(startupSize.width, startupSize.height))
    }

    function requestOpen() {
        commitPendingEdits()
        confirmDiscard(() => openDialog.open())
    }

    function requestRotate(degrees) {
        commitPendingEdits()
        doc.rotateImage(degrees)
    }

    function requestFlip(horizontal) {
        commitPendingEdits()
        doc.flipImage(horizontal)
    }

    function requestOpenUrl(url) {
        commitPendingEdits()
        confirmDiscard(() => {
            if (!doc.load(url))
                errorDialog.show(qsTr("Could not open: %1").arg(doc.lastError))
        })
    }

    function requestSave() {
        commitPendingEdits()
        if (doc.filePath.length > 0) {
            if (!doc.save())
                errorDialog.show(qsTr("Could not save: %1").arg(doc.lastError))
        } else {
            saveDialog.open()
        }
    }

    // Annotate mode: save in place and exit 0 so omapaint-edit copies the
    // result to the clipboard.
    function requestDone() {
        commitPendingEdits()
        if (doc.filePath.length === 0) {
            saveDialog.open()
            return
        }
        if (doc.save())
            Qt.exit(0)
        else
            errorDialog.show(qsTr("Could not save: %1").arg(doc.lastError))
    }

    onClosing: (close) => {
        if (doc.dirty) {
            close.accepted = false
            // Discarding in annotate mode exits non-zero so the wrapper does
            // not copy a stale file to the clipboard.
            confirmDiscard(() => annotateMode ? Qt.exit(1) : Qt.quit())
        }
    }

    header: EditorToolBar {
        doc: doc
        canvas: canvas
        annotate: window.annotateMode
        onDoneRequested: window.requestDone()
        onNewRequested: window.requestNew()
        onOpenRequested: window.requestOpen()
        onSaveRequested: window.requestSave()
        onSaveAsRequested: { window.commitPendingEdits(); saveDialog.open() }
        onResizeRequested: (mode) => resizeDialog.openFor(mode)
        onRotateRequested: (degrees) => window.requestRotate(degrees)
        onFlipRequested: (horizontal) => window.requestFlip(horizontal)
        onFontRequested: fontDialog.open()
        onAboutRequested: aboutDialog.open()
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

            // The text tool's editable-before-commit box. WYSIWYG: same font
            // scaled by zoom, same color; commits into the image via C++.
            TextArea {
                id: textOverlay
                visible: false
                property point imagePos
                x: canvas.viewOrigin.x + imagePos.x * canvas.zoom
                y: canvas.viewOrigin.y + imagePos.y * canvas.zoom
                font.family: canvas.textFont.family
                font.pixelSize: Math.max(4, canvas.textFont.pixelSize * canvas.zoom)
                font.bold: canvas.textFont.bold
                font.italic: canvas.textFont.italic
                color: canvas.foregroundColor
                padding: 0
                background: Rectangle {
                    color: "transparent"
                    border.color: "#888888"
                    border.width: 1
                }

                function commit() {
                    if (!visible)
                        return
                    canvas.commitText(Qt.point(imagePos.x, imagePos.y), text)
                    visible = false
                    text = ""
                }

                function cancel() {
                    visible = false
                    text = ""
                }

                Keys.onEscapePressed: cancel()
            }

            Connections {
                target: canvas
                function onTextEditRequested(imagePos) {
                    textOverlay.commit()
                    textOverlay.imagePos = imagePos
                    textOverlay.visible = true
                    textOverlay.forceActiveFocus()
                }
                function onToolChanged() {
                    textOverlay.commit()
                }
            }
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (drop) => {
            if (drop.urls.length === 0)
                return
            drop.acceptProposedAction()
            // confirmDiscard defers the action past the drop event, so take
            // a copy of the url now.
            const url = drop.urls[0]
            window.requestOpenUrl(url)
        }
    }

    AboutDialog {
        id: aboutDialog
        canvas: canvas
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

    FontDialog {
        id: fontDialog
        selectedFont: canvas.textFont
        onAccepted: canvas.textFont = selectedFont
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
    Shortcut { sequences: [StandardKey.Paste]; enabled: !window.typing; onActivated: canvas.paste() }
    Shortcut { sequences: [StandardKey.SelectAll]; enabled: !window.typing; onActivated: canvas.selectAll() }
    Shortcut { sequences: [StandardKey.Delete]; enabled: !window.typing; onActivated: canvas.deleteSelection() }
    Shortcut { sequence: "Escape"; enabled: !window.typing; onActivated: canvas.escape() }

    Shortcut { sequence: "P"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Pencil }
    Shortcut { sequence: "B"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Brush }
    Shortcut { sequence: "E"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Eraser }
    Shortcut { sequence: "L"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Line }
    Shortcut { sequence: "R"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Rectangle }
    Shortcut { sequence: "O"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Ellipse }
    Shortcut { sequence: "F"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Fill }
    Shortcut { sequence: "I"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Eyedropper }
    Shortcut { sequence: "S"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Selection }
    Shortcut { sequence: "A"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Arrow }
    Shortcut { sequence: "T"; enabled: !window.typing; onActivated: canvas.tool = CanvasItem.Text }
    Shortcut { sequence: "X"; enabled: !window.typing; onActivated: canvas.swapColors() }

    Shortcut { sequence: "["; enabled: !window.typing; onActivated: canvas.brushSize = canvas.brushSize - 1 }
    Shortcut { sequence: "]"; enabled: !window.typing; onActivated: canvas.brushSize = canvas.brushSize + 1 }

    Shortcut { sequences: ["+", "="]; enabled: !window.typing; onActivated: canvas.zoomIn() }
    Shortcut { sequence: "-"; enabled: !window.typing; onActivated: canvas.zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: canvas.resetZoom() }
    Shortcut {
        sequences: ["Ctrl+Return", "Ctrl+Enter"]
        enabled: window.annotateMode
        onActivated: window.requestDone()
    }
    Shortcut { sequence: "G"; enabled: !window.typing; onActivated: canvas.pixelGrid = !canvas.pixelGrid }
    Shortcut { sequence: "Ctrl+R"; onActivated: window.requestRotate(90) }
    Shortcut { sequence: "F1"; onActivated: aboutDialog.open() }
    Shortcut { sequence: "Ctrl+Shift+R"; onActivated: window.requestRotate(-90) }
}
