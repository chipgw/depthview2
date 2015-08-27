import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1

Rectangle {
    id: root
    color: "black"

    property real cellWidth: 320
    property real cellHeight: 240

    signal fileOpened(url fileURL)

    property FolderListModel model

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

                    onDoubleClicked: {
                        fileRect.color = "#888866"
                        if(fileIsDir)
                            root.folder = fileURL
                        else
                            root.fileOpened(fileURL)
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

    GridView {
        anchors.fill: parent
        model: root.model
        delegate: fileComponent

        /* So that it is the same as the delegate. */
        cellWidth: root.cellWidth
        cellHeight: root.cellHeight
    }
}

