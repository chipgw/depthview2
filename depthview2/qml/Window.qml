import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.0
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

        /* Keep the video slider updated. */
        onVideoPositionChanged: {
            playbackSlider.videoPos = image.videoPosition
            playbackSlider.value = playbackSlider.videoPos
        }
    }

    Item {
        anchors.fill: parent

        ToolBar {
            id: topMenu
            anchors {
                /* Fill the top edge of the screen. */
                top: parent.top
                left: parent.left
                right: parent.right
            }

            state: fakeCursor.y < 128 || modeSelector.popup.visible || touchTimer.running ? "" : "HIDDEN"

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
                ComboBox {
                    id: modeSelector

                    /* TODO - Doesn't work right with plugin modes. */
                    currentIndex: DepthView.drawMode

                    model: DepthView.modes

                    onActivated: {
                        if (index < DrawMode.Plugin)
                            DepthView.drawMode = index
                        else {
                            DepthView.drawMode = DrawMode.Plugin
                            DepthView.pluginMode = currentText
                        }
                    }
                }

                ToolButton {
                    text: "Open"

                    onClicked: fileBrowser.visible = true
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

                ToolButton {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Anamorphic"

                    checkable: true
                    checked: DepthView.anamorphicDualView

                    onCheckedChanged: DepthView.anamorphicDualView = checked
                }

                ToolButton {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Mirror Left"

                    checkable: true
                    checked: DepthView.mirrorLeft

                    onCheckedChanged: DepthView.mirrorLeft = checked
                }

                ToolButton {
                    visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                    text: "Mirror Right"

                    checkable: true
                    checked: DepthView.mirrorRight

                    onCheckedChanged: DepthView.mirrorRight = checked
                }

                ToolButton {
                    id: fullscreenCheckBox
                    text: "Fullscreen"

                    checkable: true
                    checked: DepthView.fullscreen

                    onCheckedChanged: DepthView.fullscreen = checked

                    /* Maintain the correct state if it changes by some other means. */
                    Connections {
                        target: DepthView

                        onFullscreenChanged: fullscreenCheckBox.checked = fullscreen
                    }
                }
            }
        }

        ToolBar {
            id: bottomMenu
            anchors {
                /* Fill the bottom edge of the screen. */
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }

            state: (root.height - fakeCursor.y) < 128 || sourceMode.popup.visible || volumePopup.visible || touchTimer.running ? "" : "HIDDEN"

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

            ColumnLayout {
                width: parent.width

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

                        to: image.videoDuration

                        /* Use this property to keep track of when value changes via binding. */
                        property int videoPos

                        /* If the value isn't the same as the tracked position then it changed by user input. */
                        onValueChanged: if (value != videoPos) image.seek(value)
                    }
                    Label {
                        /* The time remaining. */
                        text: "-" + image.timeString(image.videoDuration - image.videoPosition)

                        /* When the video is loading the duration is -1, which just looks odd. */
                        visible:  image.videoDuration > 0
                    }
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    height: childrenRect.height

                    RowLayout {
                        anchors.left: parent.left

                        ToolButton {
                            text: "About"

                            onClicked: aboutBox.open()
                        }

                        ToolButton {
                            text: "Info"

                            onClicked: mediaInfoBox.open()
                        }

                        ComboBox {
                            id: sourceMode

                            textRole: "text"
                            model: ListModel {
                                ListElement { text: "Side-by-Side"; mode: SourceMode.SidebySide }
                                ListElement { text: "Side-by-Side Anamorphic"; mode: SourceMode.SidebySideAnamorphic }
                                ListElement { text: "Top/Bottom"; mode: SourceMode.TopBottom }
                                ListElement { text: "Top/Bottom Anamorphic"; mode: SourceMode.TopBottomAnamorphic }
                                ListElement { text: "Mono"; mode: SourceMode.Mono }
                            }

                            visible: image.isVideo
                            currentIndex: image.videoMode

                            onActivated: image.videoMode = model.get(index).mode
                        }
                    }

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter

                        ToolButton {
                            text: "<"

                            onClicked: image.prevFile()
                        }

                        ToolButton {
                            enabled: image.isVideo
                            visible: image.isVideo

                            text: image.isPlaying ? "Pause" : "Play"

                            onClicked: image.playPause()
                        }

                        ToolButton {
                            text: ">"

                            onClicked: image.nextFile()
                        }
                    }

                    RowLayout {
                        id: zoomButtons

                        anchors.right: parent.right

                        function updateZoom() {
                            zoomFitButton.checked = image.zoom == -1
                            zoom100Button.checked = image.zoom == 1
                        }

                        /* Not a "zoom button" but goes in the same corner. */
                        ToolButton {
                            id: volumeButton

                            text: "Volume"
                            visible: image.isVideo

                            /* It is only possible to click this when the popup is closed. */
                            onClicked: volumePopup.open()

                            Popup {
                                id: volumePopup

                                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                                Slider {
                                    orientation: Qt.Vertical

                                    /* Init to the default value. */
                                    value: image.videoVolume

                                    onValueChanged: image.videoVolume = value
                                }
                            }
                        }

                        ToolButton {
                            id: zoomFitButton
                            text: "Fit"

                            checkable: true
                            checked: image.zoom == -1

                            onCheckedChanged: {
                                /* If this button was checked, set the zoom value to -1. */
                                if (checked)
                                    image.zoom = -1;

                                /* Either way, update the checked state of both buttons.
                                 * (If checked was set to false via mouse but the zoom is still -1 this will set it to true again.) */
                                zoomButtons.updateZoom()
                            }
                        }
                        ToolButton {
                            id: zoom100Button
                            text: "1:1"

                            checkable: true
                            checked: image.zoom == 1

                            onCheckedChanged: {
                                /* If this button was checked, set the zoom value to 1. */
                                if (checked)
                                    image.zoom = 1;

                                /* Either way, update the checked state of both buttons.
                                 * (If checked was set to false via mouse but the zoom is still 1 this will set it to true again.) */
                                zoomButtons.updateZoom()
                            }
                        }
                    }
                }
            }
        }

        Popup {
            id: aboutBox

            /* No anchors for some reason... */
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            Label {
                id: aboutLabel

                text: "<h1>DepthView " + DepthView.versionString() +
                      "</h1><p>DepthView is a basic application for viewing stereo 3D image files.</p>" +
                      "<p>DepthView website: <a href=\"https://github.com/chipgw/depthview2\">github.com/chipgw/depthview2</a></p>" +
                      "<p>Please report any bugs at: " +
                      "<a href=\"https://github.com/chipgw/depthview2/issues\">github.com/chipgw/depthview2/issues</a></p>" +
                      "<hr>"

                onLinkActivated: Qt.openUrlExternally(link)

                textFormat: Text.RichText
            }
        }
        Popup {
            id: mediaInfoBox

            /* No anchors for some reason... */
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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
        visible: mouseTimer.running || fileBrowser.visible || topMenu.state == "" || bottomMenu.state == ""

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

            onTouchEvent: touchTimer.restart()
        }

        /* Popups create their item as a child of the window's contentItem, which is the parent of the root item.
         * Thus, in order to appear above them the cursor must be parented to the same item AND have a higher z. */
        parent: root.parent
        z: 12000
    }

    Shortcut {
        sequence: StandardKey.Open
        context: Qt.ApplicationShortcut

        onActivated: fileBrowser.visible = true
    }

    Shortcut {
        sequence: "Right"
        context: Qt.ApplicationShortcut

        enabled: !fileBrowser.visible

        onActivated: image.nextFile()
    }
    Shortcut {
        sequence: "Left"
        context: Qt.ApplicationShortcut

        enabled: !fileBrowser.visible

        onActivated: image.prevFile()
    }
    Shortcut {
        sequence: "Space"
        context: Qt.ApplicationShortcut

        enabled: image.isVideo

        onActivated: image.playPause()
    }
    Shortcut {
        sequence: StandardKey.HelpContents
        context: Qt.ApplicationShortcut

        onActivated: aboutBox.visible = true
    }
    Shortcut {
        sequence: StandardKey.FullScreen
        context: Qt.ApplicationShortcut

        onActivated: DepthView.fullscreen = !DepthView.fullscreen
    }
}
