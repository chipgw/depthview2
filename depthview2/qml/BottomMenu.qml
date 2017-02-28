import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.0
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

    readonly property bool isMenuOpen: sourceMode.visible || volumePopup.visible

    function closeMenus() {
        sourceMode.close()
        volumePopup.close()
    }

    /* Visible when any of the menus are open, when no file is open, or when a video is paused. */
    state: forceOpen || isMenuOpen || FolderListing.currentFile.length < 1 || (FolderListing.currentFileIsVideo && !image.isPlaying) ? "" : "HIDDEN"

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
                    visible: !FolderListing.currentFileIsStereoImage
                    onClicked: sourceMode.open()

                    font: googleMaterialFont
                    /* TODO - I'm not sure this icon is clear enough, but it's the best fit I found.
                             * Perhaps I should make my own, and make icons for the modes themselves... */
                    text: "\ue8b9"

                    Menu {
                        id: sourceMode
                        y: -height

                        /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                        ColumnLayout {
                            Repeater {
                                model: ListModel {
                                    ListElement { text: qsTr("Side-by-Side"); mode: SourceMode.SidebySide }
                                    ListElement { text: qsTr("Side-by-Side Anamorphic"); mode: SourceMode.SidebySideAnamorphic }
                                    ListElement { text: qsTr("Top/Bottom"); mode: SourceMode.TopBottom }
                                    ListElement { text: qsTr("Top/Bottom Anamorphic"); mode: SourceMode.TopBottomAnamorphic }
                                    ListElement { text: qsTr("Mono"); mode: SourceMode.Mono }
                                }

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

                /* Not a "zoom button" but goes in the same corner. */
                ToolButton {
                    id: volumeButton

                    font: googleMaterialFont

                    /* "volume_up", "volume_down", & "volume_off", respectively. */
                    text: image.videoVolume > 0.5 ? "\ue050" : image.videoVolume > 0.0 ? "\ue04d" : "\ue04f"
                    visible: FolderListing.currentFileIsVideo

                    /* It is only possible to click this when the popup is closed. */
                    onClicked: volumePopup.open()

                    Popup {
                        id: volumePopup
                        y: -height

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
