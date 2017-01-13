import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Popup {
    id: root

    Page {
        anchors.fill: parent

        header: ToolBar {
            Row {
                anchors.fill: parent
                Repeater {
                    model: swipe.contentChildren

                    ToolButton {
                        text: modelData.title

                        onClicked: swipe.currentIndex = index

                        checked: swipe.currentIndex === index
                    }
                }
            }
        }

        footer: ToolBar {
            RowLayout {
                anchors.fill: parent

                Item {
                    Layout.fillWidth: true
                }
                ToolButton {
                    id: buttonAccept
                    text: qsTr("Ok")

                    onClicked: {
                        DepthView.saveWindowState = saveWindowStateCheckBox.checked
                        DepthView.startupFileBrowser = startupFileBrowserCheckBox.checked

                        root.close()
                    }
                }

                ToolButton {
                    id: buttonCancel
                    text: qsTr("Cancel")

                    onClicked: {
                        saveWindowStateCheckBox.checked = DepthView.saveWindowState
                        startupFileBrowserCheckBox.checked = DepthView.startupFileBrowser

                        root.close()
                    }
                }
            }
        }

        Item {
            anchors.fill: parent

            clip: true

            SwipeView {
                id: swipe
                anchors.fill: parent

                Column {
                    readonly property string title: "General Settings"

                    CheckBox {
                        id: saveWindowStateCheckBox
                        text: qsTr("Remember Window State")

                        checked: DepthView.saveWindowState
                    }

                    CheckBox {
                        id: startupFileBrowserCheckBox
                        text: qsTr("Show File Browser at Startup")

                        checked: DepthView.startupFileBrowser
                    }
                }

                Repeater {
                    model: DepthView.pluginConfigMenus

                    Item {
                        readonly property string title: modelData.title
                        children: [modelData]
                    }
                }
            }

            PageIndicator {
                currentIndex: swipe.currentIndex
                count: swipe.count

                visible: count > 1

                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
