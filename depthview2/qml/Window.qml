import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import DepthView 2.0
import Qt.labs.folderlistmodel 2.1

Rectangle {
    id: root
    color: "black"

    FolderListModel {
        id: folderModel
        nameFilters: ["*.pns", "*.jps",
            /* TODO - What other video types can we do? */
            "*.avi", "*.mp4", "*.m4v", "*.mkv"]

        /* This might be a good idea to make into a setting... */
        showDirsFirst: true
    }

    ImageViewer {
        id: image
        anchors.fill: parent

        model: folderModel

        onZoomChanged: zoomButtons.updateZoom()
    }

    Item {
        anchors {
            fill: parent
            leftMargin: 12
            rightMargin: 12
            topMargin: 2
            bottomMargin: 2
        }

        /* Put all interface items a bit above the screen. */
        transform: Translate {
            x: DepthView.isLeft ? 8 : -8
        }

        ToolBar {
            id: topMenu
            anchors.top: parent.top

            state: (fakeCursor.y < 128 &&  mouseTimer.running) || modeSelector.popupVisible || touchTimer.running ? "" : "HIDDEN"

            states: [
                State {
                    name: "HIDDEN"
                    PropertyChanges { target: topMenu; anchors.topMargin: -topMenu.height }
                }
            ]

            transitions: [
                Transition {
                    to: "*"
                    NumberAnimation {
                        target: topMenu
                        properties: "anchors.topMargin"
                        duration: 200
                    }
                }
            ]

            RowLayout {
                anchors {
                    margins: 4
                    top: parent.top
                    left: parent.left
                }

                ExclusiveGroup { id: drawModeRadioGroup }

                RadioMenu {
                    id: modeSelector

                    text: "Pick Mode"

                    Repeater {
                        id: modeList
                        model: ListModel {
                            ListElement { text: "Anaglyph"; mode: DrawMode.Anaglyph }
                            ListElement { text: "Side-by-Side"; mode: DrawMode.SidebySide }
                            ListElement { text: "Top/Bottom"; mode: DrawMode.TopBottom }
                            ListElement { text: "Mono Left"; mode: DrawMode.MonoLeft }
                            ListElement { text: "Mono Right"; mode: DrawMode.MonoRight }
                        }
                        RadioButton {
                            text: model.text
                            exclusiveGroup: drawModeRadioGroup
                            checked: DepthView.drawMode === model.mode

                            onCheckedChanged:
                                if (checked) {
                                    DepthView.drawMode = model.mode
                                    modeSelector.popupVisible = false
                                }
                        }
                    }
                    Repeater {
                        id: pluginModeList
                        model: DepthView.getPluginModes()

                        RadioButton {
                            text: modelData
                            exclusiveGroup: drawModeRadioGroup

                            onCheckedChanged:
                                if (checked) {
                                    DepthView.drawMode = DrawMode.Plugin
                                    DepthView.pluginMode = modelData
                                    modeSelector.popupVisible = false
                                }
                        }
                    }
                }

                Button {
                    text: "Open"

                    onClicked: fileBrowser.visible = true

                    Shortcut {
                        key: [StandardKey.Open]
                    }
                }

                Label {
                    text: "Grey Factor:"

                    visible: DepthView.drawMode === DrawMode.Anaglyph
                }

                Slider {
                    value: DepthView.greyFac
                    visible: DepthView.drawMode === DrawMode.Anaglyph

                    onValueChanged: DepthView.greyFac = value
                }

                CheckBox {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Anamorphic"

                    checked: DepthView.anamorphicDualView

                    onCheckedChanged: DepthView.anamorphicDualView = checked
                }

                CheckBox {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Mirror Left"

                    checked: DepthView.mirrorLeft

                    onCheckedChanged: DepthView.mirrorLeft = checked
                }

                CheckBox {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Mirror Right"

                    checked: DepthView.mirrorRight

                    onCheckedChanged: DepthView.mirrorRight = checked
                }

                CheckBox {
                    id: fullscreenCheckBox
                    text: "Fullscreen"

                    checked: DepthView.fullscreen

                    onCheckedChanged: DepthView.fullscreen = checked

                    /* Maintain the correct state if it changes by some other means. */
                    Connections {
                        target: DepthView

                        onFullscreenChanged: fullscreenCheckBox.checked = fullscreen
                    }

                    Shortcut {
                        key: [StandardKey.FullScreen, "Ctrl+F"]
                    }
                }
            }
        }

        ToolBar {
            id: bottomMenu
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }

            state: ((root.height - fakeCursor.y) < 128 && mouseTimer.running) || sourceMode.popupVisible || touchTimer.running ? "" : "HIDDEN"

            states: [
                State {
                    name: "HIDDEN"
                    PropertyChanges { target: bottomMenu; anchors.bottomMargin: -bottomMenu.height }
                }
            ]

            transitions: [
                Transition {
                    to: "*"
                    NumberAnimation {
                        target: bottomMenu
                        properties: "anchors.bottomMargin"
                        duration: 200
                    }
                }
            ]

            Item {
                width: parent.width
                height: childrenRect.height

                RowLayout {
                    id: playbackControls

                    function timeString(ms) {
                        /* Turn a time value in milliseconds into a string in the format:  "h:mm:ss". */
                        return Math.floor(ms / 3600000) + ":" + ("0" + Math.floor(ms / 60000) % 60).slice(-2) + ":" + ("0" + Math.floor(ms / 1000) % 60).slice(-2)
                    }

                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    /* Only show if currently on a video. */
                    visible: image.isVideo
                    enabled: image.isVideo

                    Label {
                        /* Show the time elapsed. */
                        text: playbackControls.timeString(image.videoPosition)

                        /* When the video is loading the duration is -1, which just looks odd. */
                        visible:  image.videoDuration > 0
                    }
                    Slider {
                        id: playbackSlider

                        Layout.fillWidth: true

                        maximumValue: image.videoDuration

                        /* Use this property to keep track of when value changes via binding. */
                        property int videoPos

                        /* If the value isn't the same as the tracked position then it changed by user input. */
                        onValueChanged: if (value != videoPos) image.seek(value)

                        Connections {
                            target: image

                            onVideoPositionChanged: {
                                playbackSlider.videoPos = image.videoPosition
                                playbackSlider.value = playbackSlider.videoPos
                            }
                        }
                    }
                    Label {
                        /* The time remaining. */
                        text: "-" + playbackControls.timeString(image.videoDuration - image.videoPosition)

                        /* When the video is loading the duration is -1, which just looks odd. */
                        visible:  image.videoDuration > 0
                    }
                }

                RowLayout {
                    id: sourceModeControls
                    anchors {
                        margins: 4
                        left: parent.left
                        top: playbackControls.bottom
                    }

                    ExclusiveGroup { id: sourceModeRadioGroup }

                    RadioMenu {
                        id: sourceMode

                        text: "Source Mode"

                        visible: image.isVideo

                        onTop: true

                        Repeater {
                            id: sourceModeList
                            model: ListModel {
                                ListElement { text: "Side-by-Side"; mode: SourceMode.SidebySide }
                                ListElement { text: "Side-by-Side Anamorphic"; mode: SourceMode.SidebySideAnamorphic }
                                ListElement { text: "Top/Bottom"; mode: SourceMode.TopBottom }
                                ListElement { text: "Top/Bottom Anamorphic"; mode: SourceMode.TopBottomAnamorphic }
                                ListElement { text: "Mono"; mode: SourceMode.Mono }
                            }
                            RadioButton {
                                text: model.text
                                exclusiveGroup: sourceModeRadioGroup
                                checked: image.videoMode === model.mode

                                onCheckedChanged:
                                    if (checked) {
                                        image.videoMode = model.mode
                                        sourceMode.popupVisible = false
                                    }
                            }
                        }
                    }
                }

                RowLayout {
                    id: navigationButtons
                    anchors {
                        margins: 4
                        horizontalCenter: parent.horizontalCenter
                        top: playbackControls.bottom
                    }

                    Button {
                        text: "<"

                        onClicked: image.prevFile()

                        Shortcut {
                            key: ["Left"]
                        }
                    }

                    Button {
                        enabled: image.isVideo
                        visible: image.isVideo

                        text: "Play/Pause"

                        onClicked: image.playPause()

                        Shortcut {
                            key: ["Space"]
                        }
                    }

                    Button {
                        text: ">"

                        onClicked: image.nextFile()

                        Shortcut {
                            key: ["Right"]
                        }
                    }
                }
                RowLayout {
                    id: zoomButtons

                    anchors {
                        margins: 4
                        right: parent.right
                        top: playbackControls.bottom
                    }

                    function updateZoom() {
                        zoomFitButton.checked = image.zoom == -1
                        zoom100Button.checked = image.zoom == 1
                    }

                    Button {
                        id: zoomFitButton
                        text: "Fit"

                        checkable: true
                        checked: image.zoom == -1

                        onClicked: { image.zoom = -1; zoomButtons.updateZoom() }
                    }
                    Button {
                        id: zoom100Button
                        text: "1:1"

                        checkable: true
                        checked: image.zoom == 1

                        onClicked: { image.zoom = 1; zoomButtons.updateZoom() }
                    }
                }
            }
        }
    }

    FileBrowser {
        id: fileBrowser
        anchors.fill: parent
        visible: false
        enabled: visible

        model: folderModel

        onFileOpened: {
            visible = false
            image.currentIndex = index
            image.updateImage()
        }
    }

    Image {
        id: fakeCursor
        source: "qrc:/images/cursor.png"

        /* Visible when the timer is running or UI is visible. */
        visible: mouseTimer.running || fileBrowser.visible

        Timer {
            id: mouseTimer
            interval: 4000
        }

        /* Use a separate timer for touchscreen events so that it doesn't show the cursor. */
        Timer {
            id: touchTimer
            interval: 2000
        }

        Connections {
            target: DepthView

            onMouseMoved: {
                /* The x coordinate needs adjusting so that the point ends up in the right place. */
                fakeCursor.x = pos.x - 8
                fakeCursor.y = pos.y

                mouseTimer.restart()
            }

            onTouchEvent: {
                touchTimer.restart()
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DepthView.isLeft ? 10 : -10
        }
    }
}
