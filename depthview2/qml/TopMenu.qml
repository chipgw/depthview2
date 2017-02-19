import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.0

ToolBar {
    id: topMenu

    anchors {
        /* Fill the top edge of the screen. */
        top: parent.top
        left: parent.left
        right: parent.right
    }

    property bool forceOpen

    property bool isMenuOpen: fileMenu.visible || modeMenu.visible

    function closeMenus() {
        fileMenu.close()
        modeMenu.close()
    }

    /* Visible when any of the menus are open, when no file is open, or when a video is paused. */
    state: forceOpen || isMenuOpen || FolderListing.currentFile.length < 1 || (FolderListing.currentFileIsVideo && !image.isPlaying) ? "" : "HIDDEN"

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

                /* This layout avoids a situation where they all end up jumbled one on top of the other for some reason... */
                ColumnLayout {
                    Repeater {
                        id: modeList
                        model: ListModel {
                            ListElement { text: qsTr("Anaglyph"); mode: DrawMode.Anaglyph }
                            ListElement { text: qsTr("Side-by-Side"); mode: DrawMode.SidebySide }
                            ListElement { text: qsTr("Top/Bottom"); mode: DrawMode.TopBottom }
                            ListElement { text: qsTr("Interlaced Horizontal"); mode: DrawMode.InterlacedH }
                            ListElement { text: qsTr("Interlaced Vertical"); mode: DrawMode.InterlacedV }
                            ListElement { text: qsTr("Checkerboard"); mode: DrawMode.Checkerboard }
                            ListElement { text: qsTr("Mono"); mode: DrawMode.Mono }
                        }
                        MenuItem {
                            text: model.text
                            font: uiTextFont

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
                            font: uiTextFont

                            checked: DepthView.drawMode === DrawMode.Plugin && DepthView.pluginMode === modelData

                            onCheckedChanged:
                                /* When checked we set the mode to Plugin and use the button text as the plugin mode. */
                                if (checked) {
                                    DepthView.pluginMode = modelData
                                    DepthView.drawMode = DrawMode.Plugin
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
            checked: DepthView.fullscreen

            onCheckedChanged: DepthView.fullscreen = checked
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
