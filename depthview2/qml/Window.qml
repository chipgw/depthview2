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

        onFolderChanged: DepthView.pushHistory(folder)
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
            x: DepthView.isLeft ? 4 : -4
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

                PopupMenu {
                    id: modeSelector

                    root: root

                    text: "Pick Mode"

                    Repeater {
                        id: modeList
                        model: ListModel {
                            ListElement { text: "Anaglyph"; mode: DrawMode.Anaglyph }
                            ListElement { text: "Side-by-Side"; mode: DrawMode.SidebySide }
                            ListElement { text: "Top/Bottom"; mode: DrawMode.TopBottom }
                            ListElement { text: "Interlaced Horizontal"; mode: DrawMode.InterlacedH }
                            ListElement { text: "Interlaced Vertical"; mode: DrawMode.InterlacedV }
                            ListElement { text: "Checkerboard"; mode: DrawMode.Checkerboard }
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
                        model: DepthView.pluginModes

                        RadioButton {
                            text: modelData
                            exclusiveGroup: drawModeRadioGroup
                            checked: DepthView.drawMode === DrawMode.Plugin && DepthView.pluginMode == modelData

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

            state: ((root.height - fakeCursor.y) < 128 && mouseTimer.running) || sourceMode.popupVisible || volumePopup.popupVisible || touchTimer.running ? "" : "HIDDEN"

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

                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    /* Only show if currently on a video. */
                    visible: image.isVideo
                    enabled: image.isVideo

                    Label {
                        /* Show the time elapsed. */
                        text: image.timeString(image.videoPosition)

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
                        text: "-" + image.timeString(image.videoDuration - image.videoPosition)

                        /* When the video is loading the duration is -1, which just looks odd. */
                        visible:  image.videoDuration > 0
                    }
                }

                RowLayout {
                    anchors {
                        margins: 4
                        left: parent.left
                        top: playbackControls.bottom
                    }

                    Button {
                        text: "About"

                        Shortcut {
                            key: [StandardKey.HelpContents]
                        }

                        onClicked: aboutBox.visible = true
                    }

                    Button {
                        text: "Info"

                        onClicked: mediaInfoBox.visible = true
                    }

                    ExclusiveGroup { id: sourceModeRadioGroup }

                    PopupMenu {
                        id: sourceMode

                        root: root

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

                            enabled: !fileBrowser.visible
                        }
                    }

                    Button {
                        enabled: image.isVideo
                        visible: image.isVideo

                        text: image.isPlaying ? "Pause" : "Play"

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

                            enabled: !fileBrowser.visible
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

                    /* Not a "zoom button" but goes in the same corner. */
                    PopupMenu {
                        id: volumePopup

                        text: "Volume"

                        visible: image.isVideo

                        root: root

                        onTop: true

                        Slider {
                            orientation: Qt.Vertical

                            /* Init to the default value. */
                            value: image.videoVolume

                            onValueChanged: image.videoVolume = value
                        }
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

        MouseArea {
            anchors.fill: parent

            visible: aboutBox.visible | mediaInfoBox.visible
            enabled: visible

            onClicked: aboutBox.visible = mediaInfoBox.visible = false
        }
        ToolBar {
            id: aboutBox
            anchors.centerIn: parent

            visible: false
            width: aboutLabel.width + 16

            Label {
                id: aboutLabel

                text: "<h1>DepthView " + DepthView.versionString() +
                      "</h1><p>DepthView is a basic application for viewing stereo 3D image files.</p>" +
                      "<p>DepthView website: <a href=\"https://github.com/chipgw/depthview2\">github.com/chipgw/depthview2</a></p>" +
                      "<p>Please report any bugs at: " +
                      "<a href=\"https://github.com/chipgw/depthview2/issues\">github.com/chipgw/depthview2/issues</a></p>" +
                      "<hr>"

                textFormat: Text.RichText
            }
        }
        ToolBar {
            id: mediaInfoBox
            anchors.centerIn: parent

            visible: false
            width: mediaInfoLabel.width + 16

            Label {
                id: mediaInfoLabel
                text: image.mediaInfo

                textFormat: Text.RichText
            }
        }
    }

    FileBrowser {
        id: fileBrowser
        anchors.fill: parent
        visible: false
        enabled: visible

        model: folderModel

        onVisibleChanged: if (image.isPlaying) image.playPause()

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
                fakeCursor.x = pos.x
                fakeCursor.y = pos.y

                mouseTimer.restart()
            }

            onTouchEvent: {
                touchTimer.restart()
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DepthView.isLeft ? 4 : -4
        }
    }
}
