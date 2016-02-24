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

    onVisibleChanged: {
        if (visible) {
            startingFolder = model.folder

            /* This forces anything bound to model.folder to update. Namely the pathText, without this it is blank at first. */
            model.folder = ""
            model.folder = startingFolder
        }
    }

    Component {
        id: fileComponent

        /* Item to have the rectangle padded inside. */
        Item {
            /* So that it is the same as the GridView. */
            width: root.cellWidth
            height: root.cellHeight

            property bool isVideo: fileName.match(/mp4$/) || fileName.match(/avi$/) || fileName.match(/m4v$/) || fileName.match(/mkv$/)

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
                            /* If it is a video or a directory use a thumbnail from qrc. Otherwise the file should be an image itself. */
                            source: fileIsDir ? "qrc:/images/folder.pns" : isVideo ? "qrc:/images/film.pns" : fileURL

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

        Flickable {
            id: drivePanel
            height: parent.height

            contentHeight: drivePanelColumn.height

            Column {
                id: drivePanelColumn

                width: parent.width
                height: childrenRect.height

                clip: true

                Text {
                    color: "white"
                    text: "Drives:"
                }

                Repeater {
                    model: DepthView.storageDevicePaths

                    delegate: Button {
                        width: drivePanel.width
                        text: data[1]

                        property variant data: modelData.split(';')

                        onClicked: root.model.folder = DepthView.encodeURL(data[0])

                        onImplicitWidthChanged: {
                            drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                        }
                    }
                }

                Text {
                    color: "white"
                    text: "Bookmarks:"
                }

                Repeater {
                    model: DepthView.bookmarks

                    delegate: Item {
                        height: childrenRect.height
                        width: drivePanel.width

                        Button {
                            /* Fit to the parent item minus space for the "X" button on the right. */
                            anchors {
                                left: parent.left
                                right: deleteButton.left
                            }
                            text: modelData.substr(modelData.lastIndexOf("/", modelData.length - 2) + 1)

                            onClicked: root.model.folder = modelData

                            onImplicitWidthChanged: {
                                drivePanel.width = Math.max(drivePanel.width, implicitWidth + 16)
                            }
                        }
                        Button {
                            id: deleteButton

                            /* Take up 16 pixels on the right of the parent item. */
                            anchors.right: parent.right
                            width: 16

                            text: "X"

                            /* TODO - Maybe this should confirm? IDK... */
                            onClicked: DepthView.deleteBookmark(modelData)
                        }
                    }
                }

                Button {
                    anchors {
                        left: drivePanelColumn.left
                        right: drivePanelColumn.right
                    }
                    text: "Add Bookmark"

                    onClicked: DepthView.addBookmark(root.model.folder)

                    onImplicitWidthChanged: {
                        drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                    }
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
                text: "<-"

                enabled: DepthView.canGoBack

                onClicked: model.folder = DepthView.goBack()

                Shortcut {
                    key: [StandardKey.Back]
                }
            }

            Button {
                text: "->"

                enabled: DepthView.canGoForward

                onClicked: model.folder = DepthView.goForward()

                Shortcut {
                    key: [StandardKey.Forward]
                }
            }

            Button {
                text: "Up"

                /* Don't go up if there is no up to go. */
                enabled: model.parentFolder.toString().length > 0

                onClicked: model.folder = model.parentFolder
            }

            TextField {
                id: pathText

                Layout.fillWidth: true

                onAccepted: {
                    if (DepthView.dirExists(text))
                        model.folder = DepthView.encodeURL(text)
                }

                /* Android has some major issues with text input ATM, best to just leave read-only. */
                readOnly: Qt.platform.os == "android"

                text: DepthView.decodeURL(model.folder)
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

