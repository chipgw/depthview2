import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1

Rectangle {
    id: root
    color: "black"
    property url folder: ""

    signal fileOpened(url fileURL)

    FolderListModel {
        id: folderModel
        nameFilters: ["*.pns", "*.jps"]
        showDirsFirst: true
        showDotAndDotDot: true
        folder: root.folder
    }

    Component {
        id: fileComponent
        Item {
            width: 320
            height: 240
            Rectangle {
                anchors.fill: parent
                anchors.margins: 8
                id: fileRect
                clip: true
                radius: 4
                color: "transparent"
                border.color: "grey"
                border.width: 4

                MouseArea {
                    anchors.margins: 4
                    anchors.fill: parent

                    onDoubleClicked: {
                        fileRect.color = "#888866"
                        if(fileIsDir)
                            root.folder = fileURL
                        else
                            root.fileOpened(fileURL)
                    }

                    hoverEnabled: true

                    onEntered: {
                        fileRect.color = "#44444444"
                    }

                    onExited: {
                        fileRect.color = "transparent"
                    }

                    Item {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: fileNameText.top

                        StereoImage {
                            anchors.centerIn: parent
                            source: fileIsDir ? "qrc:/images/folder.pns" : fileURL
                            asynchronous: !fileIsDir;
                            sourceSize.width: parent.width * 2
                            sourceSize.height: parent.height
                        }
                    }

                    Text {
                        color: "white"
                        id: fileNameText
                        anchors.bottom: parent.bottom
                        width: parent.width
                        text: fileName
                        verticalAlignment: Text.AlignBottom
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    GridView {
        anchors.fill: parent
        model: folderModel
        delegate: fileComponent
        cellWidth: 320
        cellHeight: 240
    }
}

