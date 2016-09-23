import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.0

Rectangle {
    id: root
    color: "black"

    function updateZoom() {
        zoomFitButton.checked = image.zoom == -1
        zoom100Button.checked = image.zoom == 1
    }

    ImageViewer {
        id: image
        anchors.fill: parent

        onZoomChanged: updateZoom()
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

            /* Visible when the mouse is close, when the screen was recently touched, when any of the menus are open, or when a video is paused. */
            state: fakeCursor.y < 128 || fileMenu.visible || viewMenu.visible || modeMenu.visible || helpMenu.visible ||
                   touchTimer.running || (FolderListing.currentFileIsVideo && !image.isPlaying) ? "" : "HIDDEN"

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

                ToolButton {
                    text: "File"
                    onClicked: fileMenu.open()

                    Menu {
                        id: fileMenu
                        y: parent.height

                        MenuItem {
                            text: "Open..."

                            onTriggered: fileBrowser.visible = true
                        }

                        MenuItem {
                            text: "File Info"

                            onTriggered: mediaInfoBox.open()
                        }

                        MenuItem {
                            text: "Quit"
                            onTriggered: Qt.quit()
                        }
                    }
                }

                ToolButton {
                    text: "View"
                    onClicked: viewMenu.open()

                    Menu {
                        id: viewMenu
                        y: parent.height

                        /* The layout makes it collapse items that aren't visible. */
                        ColumnLayout {
                            MenuItem {
                                visible: DepthView.drawMode === DrawMode.Anaglyph
                                text: "Grey Factor"

                                onTriggered: greyFacPopup.open()

                                Popup {
                                    id: greyFacPopup
                                    Slider {
                                        value: DepthView.greyFac
                                        visible: DepthView.drawMode === DrawMode.Anaglyph

                                        onValueChanged: DepthView.greyFac = value
                                    }
                                }
                            }

                            MenuItem {
                                visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                                text: "Anamorphic"

                                checkable: true
                                checked: DepthView.anamorphicDualView

                                onCheckedChanged: DepthView.anamorphicDualView = checked
                            }

                            MenuItem {
                                visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                                text: "Mirror Left"

                                checkable: true
                                checked: DepthView.mirrorLeft

                                onCheckedChanged: DepthView.mirrorLeft = checked
                            }

                            MenuItem {
                                visible: DepthView.drawMode === DrawMode.SidebySide || DepthView.drawMode === DrawMode.TopBottom
                                text: "Mirror Right"

                                checkable: true
                                checked: DepthView.mirrorRight

                                onCheckedChanged: DepthView.mirrorRight = checked
                            }

                            MenuItem {
                                id: fullscreenCheckBox
                                text: "Fullscreen"

                                checkable: true
                                checked: DepthView.fullscreen

                                onCheckedChanged: DepthView.fullscreen = checked
                            }
                        }
                    }
                }

                ToolButton {
                    text: "Draw Mode"
                    onClicked: modeMenu.open()

                    Menu {
                        id: modeMenu
                        y: parent.height

                        /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                        ColumnLayout {
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
                                MenuItem {
                                    text: model.text

                                    checkable: true
                                    checked: DepthView.drawMode === model.mode

                                    onCheckedChanged:
                                        if (checked) {
                                            DepthView.drawMode = model.mode
                                            modeMenu.close()
                                        }
                                }
                            }
                            Repeater {
                                id: pluginModeList
                                model: DepthView.pluginModes

                                MenuItem {
                                    text: modelData
                                    checkable: true

                                    checked: DepthView.drawMode === DrawMode.Plugin && DepthView.pluginMode === modelData

                                    onCheckedChanged:
                                        /* When checked we set the mode to Plugin and use the button text as the plugin mode. */
                                        if (checked) {
                                            DepthView.drawMode = DrawMode.Plugin
                                            DepthView.pluginMode = modelData
                                            modeMenu.close()
                                        }
                                }
                            }
                        }
                    }
                }

                ToolButton {
                    text: "Help"
                    onClicked: helpMenu.open()

                    Menu {
                        id: helpMenu
                        y: parent.height

                        MenuItem {
                            text: "About"

                            onClicked: aboutBox.open()
                        }
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

            /* Visible when the mouse is close, when the screen was recently touched, when any of the menus are open, or when a video is paused. */
            state: (root.height - fakeCursor.y) < 128 || sourceMode.visible || volumePopup.visible || touchTimer.running || (FolderListing.currentFileIsVideo && !image.isPlaying)
                   ? "" : "HIDDEN"

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
                    visible: FolderListing.currentFileIsVideo
                    enabled: FolderListing.currentFileIsVideo

                    Label {
                        /* Show the time elapsed. */
                        text: image.timeString(image.videoPosition)

                        /* When the video is loading the duration is -1, which just looks odd. */
                        visible: image.videoDuration > 0
                    }

                    ProgressBar {
                        Layout.fillWidth: true

                        to: image.videoDuration
                        value: image.videoPosition

                        /* When the video is loading the duration is -1, so show the indeterminate progressbar. */
                        indeterminate: image.videoDuration <= 0

                        MouseArea {
                            anchors.fill: parent

                            /* mouseX * this = the position of the video at the point under the cursor.
                             * (mouseX is already relative to this object, which makes it easy.) */
                            property real screenPosToTime: image.videoDuration / width

                            /* When the mouse moves (only when pressed) or is clicked, seek to that position. */
                            onMouseXChanged: image.seek(screenPosToTime * mouseX);
                            onClicked: image.seek(screenPosToTime * mouseX);

                            acceptedButtons: Qt.LeftButton
                        }
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
                            visible: FolderListing.currentFileIsVideo
                            onClicked: sourceMode.open()

                            text: "Source Mode"

                            Menu {
                                id: sourceMode
                                y: -height

                                /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                                ColumnLayout {
                                    Repeater {
                                        model: ListModel {
                                            ListElement { text: "Side-by-Side"; mode: SourceMode.SidebySide }
                                            ListElement { text: "Side-by-Side Anamorphic"; mode: SourceMode.SidebySideAnamorphic }
                                            ListElement { text: "Top/Bottom"; mode: SourceMode.TopBottom }
                                            ListElement { text: "Top/Bottom Anamorphic"; mode: SourceMode.TopBottomAnamorphic }
                                            ListElement { text: "Mono"; mode: SourceMode.Mono }
                                        }

                                        MenuItem {
                                            text: model.text

                                            checkable: true
                                            checked: image.videoMode === model.mode

                                            onCheckedChanged:
                                                if (checked) {
                                                    image.videoMode = model.mode
                                                    sourceMode.close()
                                                }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        /* Navigation controls in the middle. */
                        anchors.horizontalCenter: parent.horizontalCenter

                        ToolButton {
                            text: "<"

                            onClicked: FolderListing.openPrevious()
                        }

                        ToolButton {
                            visible: FolderListing.currentFileIsVideo

                            text: image.isPlaying ? "Pause" : "Play"

                            onClicked: image.playPause()
                        }

                        ToolButton {
                            text: ">"

                            onClicked: FolderListing.openNext()
                        }
                    }

                    RowLayout {
                        anchors.right: parent.right

                        /* Not a "zoom button" but goes in the same corner. */
                        ToolButton {
                            id: volumeButton

                            text: "Volume"
                            visible: FolderListing.currentFileIsVideo

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
                                updateZoom()
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
                                updateZoom()
                            }
                        }
                    }
                }
            }
        }

        Popup {
            id: aboutBox

            /* Anchors don't work on popups because they are appended to the window content item. */
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

                /* Allow clicking links in the window. */
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

        width: parent.width
        height: parent.height

        /* Ensure video is paused when file browser is opened. */
        onOpened: if (image.isPlaying) image.playPause()
    }

    Image {
        /* Start out off-screen so if the position doesn't get set it won't show up in the corner. */
        x: -width
        y: -height

        id: fakeCursor
        source: "qrc:/images/cursor.png"

        /* Visible when the timer is running or when the UI is visible. */
        visible: mouseTimer.running || fileBrowser.visible || topMenu.state == "" || bottomMenu.state == ""

        Timer {
            id: mouseTimer
            interval: 4000
        }

        /* Use a separate timer for touchscreen events for showing toolbars instead of the cursor. */
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

        onActivated: fileBrowser.open()
    }

    Shortcut {
        sequence: "Right"
        context: Qt.ApplicationShortcut

        enabled: !fileBrowser.visible

        onActivated: FolderListing.openNext()
    }
    Shortcut {
        sequence: "Left"
        context: Qt.ApplicationShortcut

        enabled: !fileBrowser.visible

        onActivated: FolderListing.openPrevious()
    }
    Shortcut {
        sequence: "Space"
        context: Qt.ApplicationShortcut

        enabled: FolderListing.currentFileIsVideo

        onActivated: image.playPause()
    }
    Shortcut {
        sequence: StandardKey.HelpContents
        context: Qt.ApplicationShortcut

        onActivated: aboutBox.open()
    }
    Shortcut {
        sequence: StandardKey.FullScreen
        context: Qt.ApplicationShortcut

        onActivated: DepthView.fullscreen = !DepthView.fullscreen
    }

    /* The popup close policy escape shortcut isn't working, so take care of it here. */
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: {
            /* TODO - There must be an easier way to do this... */
            if (fileBrowser.visible)
                fileBrowser.cancel()
            if (aboutBox.visible)
                aboutBox.close()
            if (mediaInfoBox.visible)
                mediaInfoBox.close()
            if (volumePopup.visible)
                volumePopup.close()
            if (fileMenu.visible)
                fileMenu.close()
            if (viewMenu.visible)
                viewMenu.close()
            if (greyFacPopup.visible)
                greyFacPopup.close()
            if (modeMenu.visible)
                modeMenu.close()
            if (helpMenu.visible)
                helpMenu.close()
            if (sourceMode.visible)
                sourceMode.close()
        }
    }
}
