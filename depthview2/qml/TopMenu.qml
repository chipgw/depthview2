import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.1
import QtQuick.Window 2.2

ToolBar {
    id: topMenu

    anchors {
        /* Fill the top edge of the screen. */
        top: parent.top
        left: parent.left
        right: parent.right
    }

    property bool forceOpen

    readonly property bool isMenuOpen: fileMenu.visible || modeMenu.visible

    function closeMenus() {
        fileMenu.close()
        modeMenu.close()
    }

    /* Visible when any of the menus are open, when no file is open, or when a video is paused. */
    state: forceOpen || isMenuOpen || FolderListing.currentFile.length < 1 || (FolderListing.currentFileIsVideo && !image.isPlaying) ? "" : "HIDDEN"

    states: [
        State {
            name: "HIDDEN"
            /* Put slightly above the edge of the screen so as to avoid leaving a line behind when hidden. */
            PropertyChanges { target: topMenu; anchors.topMargin: -topMenu.height-8 }
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
        width: parent.width

        ToolButton {
            text: qsTr("File")
            font: uiTextFont
            onClicked: fileMenu.open()

            Menu {
                id: fileMenu
                y: parent.height

                MenuItem {
                    text: qsTr("Open...")
                    font: uiTextFont

                    onTriggered: FolderListing.fileBrowserOpen = true
                }

                MenuItem {
                    text: qsTr("Take Snapshot")
                    font: uiTextFont

                    enabled: FolderListing.currentFileIsVideo
                    onTriggered: DepthView.takeSnapshot()
                }

                MenuItem {
                    text: qsTr("File Info")
                    font: uiTextFont

                    onTriggered: mediaInfoBox.open()
                }

                MenuItem {
                    text: qsTr("Quit")
                    font: uiTextFont
                    onTriggered: Qt.quit()
                }
            }
        }

        ToolButton {
            text: qsTr("Draw Mode")
            font: uiTextFont
            onClicked: modeMenu.open()

            Menu {
                id: modeMenu
                y: parent.height

                ButtonGroup {
                    buttons: modeListColumn.children
                }

                /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                ColumnLayout {
                    id: modeListColumn

                    Repeater {
                        id: modeList
                        model: ListModel {
                            ListElement { text: qsTr("Anaglyph"); mode: DrawMode.Anaglyph }
                            ListElement { text: qsTr("Side-by-Side"); mode: DrawMode.SideBySide }
                            ListElement { text: qsTr("Top/Bottom"); mode: DrawMode.TopBottom }
                            ListElement { text: qsTr("Interlaced Horizontal"); mode: DrawMode.InterlacedH }
                            ListElement { text: qsTr("Interlaced Vertical"); mode: DrawMode.InterlacedV }
                            ListElement { text: qsTr("Checkerboard"); mode: DrawMode.Checkerboard }
                            ListElement { text: qsTr("Mono"); mode: DrawMode.Mono }
                            ListElement { text: qsTr("Virtual Reality"); mode: DrawMode.VirtualReality }
                        }
                        MenuItem {
                            text: model.text
                            font: uiTextFont

                            checkable: true
                            checked: DepthView.drawMode === model.mode

                            visible: model.mode !== DrawMode.VirtualReality || (VRManager.isInited && !VRManager.isError)

                            onCheckedChanged:
                                if (checked) {
                                    DepthView.drawMode = model.mode
                                    modeMenu.close()
                                }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true;
        }

        ToolButton {
            /* "fullscreen_exit" and "fullscreen". */
            text: checked ? "\ue5d1" : "\ue5d0"
            font: googleMaterialFont

            checkable: true
            checked: root.visibility == Window.FullScreen

            onCheckedChanged: root.visibility = checked ? Window.FullScreen : Window.Maximized
        }
        ToolButton {
            /* "settings" */
            text: "\ue8b8"
            font: googleMaterialFont
            onClicked: settingsPopup.open()
        }

        ToolButton {
            /* "help" */
            text: "\ue887"
            font: googleMaterialFont
            onClicked: aboutBox.open()
        }
    }
}
