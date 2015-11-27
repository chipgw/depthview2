import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import Qt.labs.folderlistmodel 2.1
import DepthView 2.0

Rectangle {
    id: root
    color: "black"

    property real cellWidth: 320
    property real cellHeight: 240

    signal fileOpened(url fileURL, int index)

    property FolderListModel model

    property url startingFolder

    onVisibleChanged: if (visible) startingFolder = model.folder

    Component {
        id: fileComponent

        /* Item to have the rectangle padded inside. */
        Item {
            /* So that it is the same as the GridView. */
            width: root.cellWidth
            height: root.cellHeight

            /* Border/highlight rectangle. */
            Rectangle {
                id: fileRect
                anchors { fill: parent; margins: 8 }
                radius: 4
                color: "transparent"
                border { color: "grey"; width: 4 }

                MouseArea {
                    anchors { fill: parent; margins: 4 }

                    onClicked: {
                        fileRect.color = "#888866"
                        if(fileIsDir)
                            root.model.folder = fileURL
                        else
                            root.fileOpened(fileURL, index)
                    }

                    hoverEnabled: true

                    /* Highlight on mouseover. */
                    onEntered: fileRect.color = "#44444444"
                    onExited: fileRect.color = "transparent"

                    /* Item to center the thumbnail inside. */
                    Item {
                        /* Fill the parent except for on the bottom where the text is. */
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            bottom: fileNameText.top
                        }

                        /* 3D thumbnails! */
                        /* TODO - Video thumbnails. */
                        StereoImage {
                            anchors.centerIn: parent
                            /* If it is a directory use the thumbnail in qrc. Otherwise the file should be an image itself. */
                            source: fileIsDir ? "qrc:/images/folder.pns" : fileURL

                            /* Images on the filesystem should be loaded asynchronously. */
                            asynchronous: !fileIsDir;

                            /* The image should onlt be stored at the needed size. */
                            sourceSize: Qt.size(parent.width * 2, parent.height)
                        }
                    }

                    Text {
                        id: fileNameText
                        anchors.bottom: parent.bottom
                        width: parent.width

                        color: "white"
                        text: fileName

                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    SplitView {
        anchors {
            top: controls.bottom
            left:  parent.left
            right:  parent.right
            bottom:  parent.bottom
        }
        orientation: Qt.Horizontal

        Column {
            id: drivePanel
            width: 128

            clip: true

            Label {
                text: "Drives:"
            }

            Repeater {
                model: DepthView.getStorageDevicePaths()

                Timer  {
                    interval: 10000
                    running: root.visible
                    repeat: true
                    onTriggered: drivePanel.model = DepthView.getStorageDevicePaths();
                }

                delegate: Button {
                    anchors {
                        left: drivePanel.left
                        right: drivePanel.right
                    }
                    text: data[1]

                    property variant data: modelData.split(';')

                    /* If the there should only be three '/'s after "file:" in the path. */
                    onClicked: root.model.folder = (data[0].charAt(0) == '/' ? "file://" : "file:///") + data[0]
                }
            }
        }

        GridView {
            Layout.fillWidth: true

            model: root.model
            delegate: fileComponent

            /* So that it is the same as the delegate. */
            cellWidth: root.cellWidth
            cellHeight: root.cellHeight
        }
    }

    ToolBar {
        id: controls
        anchors {
            margins: 12
            top: parent.top
            left: parent.left
            right: parent.right
        }

        /* Put all interface items a bit above the screen. */
        transform: Translate {
            x: DepthView.isLeft ? 4 : -4
        }

        RowLayout {
            anchors.fill: parent

            Button {
                text: "Up"

                onClicked: {
                    /* Don't go up if there is no up to go. */
                    if (model.parentFolder.toString().length > 0)
                        model.folder = model.parentFolder
                }
            }

            TextField {
                id: pathText

                Layout.fillWidth: true

                /* TODO - Make this editable. */
                readOnly: true

                /* TODO - This is blank at first, because folder is blank.
                 * Hopefully I can find a way to get the actual absolute path... */
                text: model.folder
            }

            Button {
                text: "Cancel"

                onClicked: {
                    /* Reset to the folder that was active when the browser was first shown. */
                    model.folder = startingFolder
                    root.visible = false
                }

                Shortcut {
                    key: ["Esc"]
                }
            }
        }
    }
}

