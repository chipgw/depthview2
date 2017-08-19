import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.1
import QtAV 1.6

ToolBar {
    id: bottomMenu
    anchors {
        /* Fill the bottom edge of the screen. */
        bottom: parent.bottom
        left: parent.left
        right: parent.right
    }

    property bool forceOpen

    function updateZoom() {
        zoomFitButton.checked = image.zoom === -1
        zoom100Button.checked = image.zoom === 1
    }

    readonly property bool isMenuOpen: sourceMode.visible || volumePopup.visible || audioTracksMenu.visible

    function closeMenus() {
        sourceMode.close()
        volumePopup.close()
    }

    /* Visible when any of the menus are open, when no file is open, or when a video is paused. */
    state: forceOpen || isMenuOpen || FolderListing.currentFile.length < 1 || (FolderListing.currentFileIsVideo && !image.isPlaying) ? "" : "HIDDEN"

    states: [
        State {
            name: "HIDDEN"
            /* Put slightly below the edge of the screen so as to avoid leaving a line behind when hidden. */
            PropertyChanges { target: bottomMenu; anchors.bottomMargin: -bottomMenu.height-8 }
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
            /* A duration of 0 indicates that the video is stopped, and will messes up the progress bar thumbnail. */
            enabled: image.videoDuration > 0

            Label {
                /* Show the time elapsed. */
                text: "  " + image.timeString(image.videoPosition)

                /* When the video is loading the duration is -1, which just looks odd. */
                visible: image.videoDuration > 0
            }

            VideoProgressBar {
                Layout.fillWidth: true
            }

            Label {
                /* The time remaining. */
                text: "-" + image.timeString(image.videoDuration - image.videoPosition) + "  "

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
                    onClicked: sourceMode.open()

                    font: googleMaterialFont
                    /* TODO - I'm not sure this icon is clear enough, but it's the best fit I found.
                     * Perhaps I should make my own, and make icons for the modes themselves... */
                    text: "\ue8b9"

                    Menu {
                        id: sourceMode
                        y: -height
                        modal: true

                        MenuItem {
                            id: stereoSwapMenuItem
                            text: qsTr("Swap Stereo")
                            font: uiTextFont

                            checkable: true
                            checked: FolderListing.currentFileStereoSwap !== FolderListing.currentFileIsStereoImage

                            onCheckedChanged: FolderListing.currentFileStereoSwap = (checked !== FolderListing.currentFileIsStereoImage)
                        }

                        MenuItem {
                            id: surroundMenuItem
                            text: qsTr("360")
                            font: uiTextFont

                            checkable: true
                            checked: FolderListing.currentFileIsSurround

                            /* Surround is not available for *.jps & *.pns files. */
                            visible: !FolderListing.currentFileIsStereoImage
                            height: visible ? implicitHeight : 0

                            onCheckedChanged: FolderListing.currentFileIsSurround = checked
                        }

                        Connections {
                            target: FolderListing

                            onCurrentFileStereoSwapChanged: stereoSwapMenuItem.checked = (FolderListing.currentFileStereoSwap !== FolderListing.currentFileIsStereoImage)
                            onCurrentFileSurroundChanged: surroundMenuItem.checked = FolderListing.currentFileIsSurround
                        }

                        ButtonGroup {
                            buttons: sourceModeColumn.children
                        }

                        /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                        ColumnLayout {
                            id: sourceModeColumn

                            /* Hide and give a height of 0 when the file is a stereo image.
                             * Stereo images are always side by side, but access to swap might still be needed sometimes. */
                            visible: !FolderListing.currentFileIsStereoImage
                            height: visible ? implicitHeight : 0

                            MenuSeparator { }

                            ListModel {
                                id: allSourceModes
                                ListElement { text: qsTr("Side-by-Side"); mode: SourceMode.SidebySide }
                                ListElement { text: qsTr("Side-by-Side Anamorphic"); mode: SourceMode.SidebySideAnamorphic }
                                ListElement { text: qsTr("Top/Bottom"); mode: SourceMode.TopBottom }
                                ListElement { text: qsTr("Top/Bottom Anamorphic"); mode: SourceMode.TopBottomAnamorphic }
                                ListElement { text: qsTr("Mono"); mode: SourceMode.Mono }
                            }
                            ListModel {
                                id: surroundSourceModes
                                ListElement { text: qsTr("Side-by-Side"); mode: SourceMode.SidebySide }
                                ListElement { text: qsTr("Top/Bottom"); mode: SourceMode.TopBottom }
                                ListElement { text: qsTr("Mono"); mode: SourceMode.Mono }
                            }

                            Repeater {
                                model: FolderListing.currentFileIsSurround ? surroundSourceModes : allSourceModes

                                MenuItem {
                                    text: model.text

                                    checkable: true
                                    checked: FolderListing.currentFileStereoMode === model.mode
                                    font: uiTextFont

                                    onCheckedChanged:
                                        if (checked) {
                                            FolderListing.currentFileStereoMode = model.mode
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
                    font: googleMaterialFont
                    /* "skip_previous" */
                    text: "\ue045"

                    onClicked: FolderListing.openPrevious()
                }

                ToolButton {
                    visible: FolderListing.currentFileIsVideo

                    font: googleMaterialFont
                    /* "pause" and "play_arrow" */
                    text: image.isPlaying ? "\ue034" : "\ue037"

                    onClicked: image.playPause()
                }
                ToolButton {
                    visible: FolderListing.currentFileIsVideo

                    font: googleMaterialFont
                    /* "fast_forward" */
                    text: "\ue01f"

                    onClicked: image.fastForward()
                }

                ToolButton {
                    font: googleMaterialFont
                    /* "skip_next" */
                    text: "\ue044"

                    onClicked: FolderListing.openNext()
                }
            }

            RowLayout {
                anchors.right: parent.right

                ToolButton {
                    font: googleMaterialFont

                    /* "queue_music". */
                    text: "\ue03d"
                    /* Only show if the open video has more than a single track available. */
                    visible: FolderListing.currentFileIsVideo && image.audioTracks.length > 1

                    /* It is only possible to click this when the popup is closed. */
                    onClicked: audioTracksMenu.open()

                    Menu {
                        id: audioTracksMenu
                        y: -height
                        modal: true

                        ColumnLayout {
                            Repeater {
                                model: image.audioTracks

                                MenuItem {
                                    /* Only show language if there's actually a language to show. */
                                    text: modelData.title + (modelData.language.length > 0  ? " (" + modelData.language + ")" : "");

                                    checkable: true
                                    checked: index === image.audioTrack
                                    font: uiTextFont

                                    onCheckedChanged:
                                        if (checked) {
                                            image.audioTrack = index
                                            audioTracksMenu.close()
                                        }
                                }
                            }
                        }
                    }
                }

                ToolButton {
                    font: googleMaterialFont

                    /* "volume_up", "volume_down", & "volume_off", respectively. */
                    text: image.videoVolume > 0.5 ? "\ue050" : image.videoVolume > 0.0 ? "\ue04d" : "\ue04f"
                    visible: FolderListing.currentFileIsVideo

                    /* It is only possible to click this when the popup is closed. */
                    onClicked: volumePopup.open()

                    Popup {
                        id: volumePopup
                        y: -height
                        modal: true

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
                    text: qsTr("Fit")
                    font: uiTextFont

                    checkable: true
                    checked: image.zoom === -1

                    /* Hide when viewing surround images in VR. */
                    visible: DepthView.drawMode !== DrawMode.VirtualReality || !FolderListing.currentFileIsSurround

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
                    text: qsTr("1:1")
                    font: uiTextFont

                    checkable: true
                    checked: image.zoom === 1

                    /* Hide when viewing surround images in VR. */
                    visible: DepthView.drawMode !== DrawMode.VirtualReality || !FolderListing.currentFileIsSurround

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
