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
            x: DV.isLeft ? 8 : -8
        }

        ToolBar {
            id: topMenu
            anchors.top: parent.top

            state: fakeCursor.y < 128 &&  mouseTimer.running ? "" : "HIDDEN"

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

                Button {
                    text: "Pick Mode"

                    onClicked: modeDialog.visible = !modeDialog.visible
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

                    visible: DV.drawMode === DepthView.Anglaph
                }

                Slider {
                    value: DV.greyFac
                    visible: DV.drawMode === DepthView.Anglaph

                    onValueChanged: DV.greyFac = value
                }

                CheckBox {
                    visible: DV.drawMode === DepthView.SidebySide || DV.drawMode === DepthView.TopBottom
                    text: "Anamorphic"

                    checked: DV.anamorphicDualView

                    onCheckedChanged: DV.anamorphicDualView = checked
                }

                CheckBox {
                    visible: DV.drawMode === DepthView.SidebySide || DV.drawMode === DepthView.TopBottom
                    text: "Mirror Left"

                    checked: DV.mirrorLeft

                    onCheckedChanged: DV.mirrorLeft = checked
                }

                CheckBox {
                    visible: DV.drawMode === DepthView.SidebySide || DV.drawMode === DepthView.TopBottom
                    text: "Mirror Right"

                    checked: DV.mirrorRight

                    onCheckedChanged: DV.mirrorRight = checked
                }

                CheckBox {
                    id: fullscreenCheckBox
                    text: "Fullscreen"

                    checked: DV.fullscreen

                    onCheckedChanged: DV.fullscreen = checked

                    /* Maintain the correct state if it changes by some other means. */
                    Connections {
                        target: DV

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

            state: (root.height - fakeCursor.y) < 128 && mouseTimer.running ? "" : "HIDDEN"

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

                Slider {
                    id: playbackSlider
                    visible: image.isVideo
                    enabled: image.isVideo

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    maximumValue: image.videoDuration

                    onValueChanged: image.seek(value)

                    Binding {
                        target: playbackSlider
                        property: "value"
                        value: image.videoPosition
                    }
                }

                RowLayout {
                    height: childrenRect.height
                    id: navigationButtons
                    anchors {
                        margins: 4
                        horizontalCenter: parent.horizontalCenter
                        top: playbackSlider.bottom
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
                        top: playbackSlider.bottom
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

        GroupBox {
            id: modeDialog
            title: "Draw Mode"
            anchors.centerIn: parent

            /* Hide by default and don't enable when hidden. */
            visible: false
            enabled: visible

            ExclusiveGroup { id: drawModeRadioGroup }

            ColumnLayout {
                Repeater {
                    id: modeList
                    model: ListModel {
                        ListElement { text: "Anglaph"; mode: DepthView.Anglaph }
                        ListElement { text: "Side-by-Side"; mode: DepthView.SidebySide }
                        ListElement { text: "Top/Bottom"; mode: DepthView.TopBottom }
                        ListElement { text: "Mono Left"; mode: DepthView.MonoLeft }
                        ListElement { text: "Mono Right"; mode: DepthView.MonoRight }
                    }
                    RadioButton {
                        text: model.text
                        exclusiveGroup: drawModeRadioGroup
                        checked: DV.drawMode === model.mode

                        onCheckedChanged:
                            if (checked) {
                                DV.drawMode = model.mode
                                modeDialog.visible = false
                            }
                    }
                }
                Repeater {
                    id: pluginModeList
                    model: DV.getPluginModes()

                    RadioButton {
                        text: modelData
                        exclusiveGroup: drawModeRadioGroup

                        onCheckedChanged:
                            if (checked) {
                                DV.drawMode = DepthView.Plugin
                                DV.pluginMode = modelData
                                modeDialog.visible = false
                            }
                    }
                }
                Button {
                    text: "Close"

                    onClicked: modeDialog.visible = false
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

        Connections {
            target: DV

            onMouseMoved: {
                /* The x coordinate needs adjusting so that the point ends up in the right place. */
                fakeCursor.x = pos.x - 8
                fakeCursor.y = pos.y

                mouseTimer.restart()
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DV.isLeft ? 10 : -10
        }
    }
}
