import QtQuick 2.5
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0
import Qt.labs.folderlistmodel 2.1
import DepthView 2.0
import QtAV 1.6

Rectangle {
    id: root
    color: "black"

    property real cellWidth: 320
    property real cellHeight: 240

    /* The signal sends the index of the selected file within the model. */
    signal fileOpened(int index)

    property FolderListModel model

    property url startingFolder

    function cancel() {
        /* Reset to the folder that was active when the browser was first shown. */
        model.folder = startingFolder
        root.visible = false
    }

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
                        /* For the short time it's still visible, highlight the clicked item. */
                        fileRect.color = "#888866"
                        if(fileIsDir)
                            root.model.folder = fileURL
                        else
                            root.fileOpened(index)
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

                            visible: !isVideo
                            enabled: !isVideo

                            /* If it is a directory use a thumbnail from qrc. Otherwise the file should be an image itself. */
                            source: fileIsDir ? "qrc:/images/folder.pns" : fileURL

                            /* Images on the filesystem should be loaded asynchronously. */
                            asynchronous: !fileIsDir;

                            /* The image should only be stored at the needed size. */
                            sourceSize: Qt.size(parent.width * 2, parent.height)
                        }

                        /* Video thumbnails! */
                        /* TODO - Set up a system to detect if a video is 3D... */
                        VideoPreview {
                            anchors.fill: parent
                            file: fileURL

                            visible: isVideo
                            enabled: isVideo

                            /* Set timestamp to one minute in. */
                            Component.onCompleted: timestamp = 60000

                            /* This doesn't work because it doesn't send the signal to the thing that gets the frame. */
                            /*timestamp: 60000*/
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

    Page {
        id: page
        anchors.fill: parent

        /* Sidebar containing the drive list and bookmarks. */
        Flickable {
            id: drivePanel
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }

            Column {
                id: drivePanelColumn

                width: parent.width
                height: childrenRect.height

                Label {
                    padding: 8
                    text: "Drives:"
                }

                Repeater {
                    model: DepthView.storageDevicePaths

                    delegate: Button {
                        width: drivePanel.width
                        text: data[1]

                        /* Drive info is provided as "<path>;<display name>". */
                        property variant data: modelData.split(';')

                        onClicked: root.model.folder = DepthView.encodeURL(data[0])

                        onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                    }
                }

                Label {
                    padding: 8
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

                            onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth + 16)
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

                    onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                }
            }
        }

        Rectangle {
            color: "black"
            anchors {
                left: drivePanel.right
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            Layout.fillWidth: true

            GridView {
                anchors.fill: parent
                model: root.model
                delegate: fileComponent

                ScrollBar.vertical: ScrollBar { }

                /* So that it is the same as the delegate. */
                cellWidth: root.cellWidth
                cellHeight: root.cellHeight
            }
        }

        header: ToolBar {
            RowLayout {
                anchors.fill: parent

                ToolButton {
                    text: "<-"
                    
                    enabled: DepthView.canGoBack
                    
                    onClicked: model.folder = DepthView.goBack()
                }
                
                ToolButton {
                    text: "->"
                    
                    enabled: DepthView.canGoForward
                    
                    onClicked: model.folder = DepthView.goForward()
                }
                
                ToolButton {
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
                
                ToolButton {
                    text: "Cancel"
                    
                    onClicked: root.cancel()
                }
            }
        }
    }

    /* A mouseArea covering everythingto capture back/forward buttons. */
    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.ForwardButton | Qt.BackButton

        onClicked: {
            if (mouse.button == Qt.BackButton && DepthView.canGoBack)
                model.folder = DepthView.goBack()
            if (mouse.button == Qt.ForwardButton && DepthView.canGoForward)
                model.folder = DepthView.goForward()
        }
    }

    Shortcut {
        sequence: StandardKey.Back
        enabled: parent.visible && DepthView.canGoBack
        context: Qt.ApplicationShortcut
        onActivated: model.folder = DepthView.goBack()
    }

    Shortcut {
        sequence: StandardKey.Forward
        enabled: parent.visible && DepthView.canGoForward
        context: Qt.ApplicationShortcut
        onActivated: model.folder = DepthView.goForward()
    }

    Shortcut {
        sequence: "Esc"
        enabled: parent.visible
        context: Qt.ApplicationShortcut
        onActivated: root.cancel()
    }
}

