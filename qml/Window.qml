import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import DepthView 2.0
import Qt.labs.folderlistmodel 2.1

Rectangle {
    color: "black"

    FolderListModel {
        id: folderModel
        nameFilters: ["*.pns", "*.jps"]

        /* This might be a good idea to make into a setting... */
        showDirsFirst: true

        /* I wish I could just to ".." and not need "." with it... */
        showDotAndDotDot: true
    }

    ImageViewer {
        id: image
        anchors.fill: parent
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
            id: menuRect

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
                    visible: DV.drawMode == DepthView.SidebySide || DV.drawMode == DepthView.TopBottom
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
            image.source = fileURL
        }
    }

    Image {
        id: fakeCursor
        source: "qrc:/images/cursor.png"

        Connections {
            target: DV

            onMouseMoved: {
                /* The x coordinate needs adjusting so that the point ends up in the right place. */
                fakeCursor.x = pos.x - 8
                fakeCursor.y = pos.y
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DV.isLeft ? 10 : -10
        }
    }
}

