import QtQuick 2.5
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0
import DepthView 2.0
import QtAV 1.6

Popup {
    id: root

    padding: 0

    property real cellWidth: 320
    property real cellHeight: 240

    property url startingFolder

    function cancel() {
        /* Reset to the folder that was active when the browser was first shown. */
        FolderListing.currentDir = startingFolder
        close()
    }

    onOpened:
        startingFolder = FolderListing.currentDir

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
                        /* For the short time it's still visible, highlight the clicked item. */
                        fileRect.color = "#888844"
                        if (fileIsDir)
                            FolderListing.currentDir = fileURL
                        else {
                            FolderListing.openFile(fileURL)
                            close();
                        }
                    }

                    hoverEnabled: true

                    /* Highlight on mouseover. */
                    onEntered: fileRect.color = "#44444488"
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

                        Loader {
                            anchors.fill: parent
                            sourceComponent: fileIsVideo ? videoThumbComponent : imageThumbComponent
                        }

                        Component {
                            id: imageThumbComponent

                            Item {
                                /* 3D thumbnails! */
                                StereoImage {
                                    anchors.centerIn: parent

                                    /* If it is a directory use a thumbnail from qrc. Otherwise the file should be an image itself. */
                                    source: fileIsDir ? "qrc:/images/folder.pns" : fileURL

                                    /* Images on the filesystem should be loaded asynchronously. */
                                    asynchronous: !fileIsDir;

                                    /* The image should only be stored at the needed size. */
                                    sourceSize: Qt.size(parent.width * 2, parent.height)
                                }
                            }
                        }

                        Component {
                            id: videoThumbComponent

                            /* Video thumbnails! */
                            /* TODO - Set up a system to detect if a video is 3D... */
                            VideoPreview {
                                file: fileURL

                                /* Set timestamp to one minute in. */
                                Component.onCompleted: timestamp = 60000

                                /* This doesn't work because it doesn't send the signal to the thing that gets the frame. */
                                /*timestamp: 60000*/
                            }
                        }
                    }

                    Text {
                        id: fileNameText

                        /* On the bottom of the item, fill the width but be automatic height. */
                        anchors.bottom: parent.bottom
                        width: parent.width

                        color: "white"
                        text: fileName

                        horizontalAlignment: Text.AlignHCenter

                        /* We wrap text and then when it is more than one line the thumbnail will automatically resize to compensate. */
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
                    model: FolderListing.storageDevicePaths

                    delegate: Button {
                        width: drivePanel.width
                        text: data[1]

                        /* Drive info is provided as "<path>;<display name>". */
                        property variant data: modelData.split(';')

                        onClicked: FolderListing.currentDir = FolderListing.encodeURL(data[0])

                        /* Whenever the text width changes, make sure that the panel is large enough to fit. */
                        onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                    }
                }

                Label {
                    padding: 8
                    text: "Bookmarks:"
                }

                Repeater {
                    model: FolderListing.bookmarks

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

                            onClicked: FolderListing.currentDir = modelData

                            /* Whenever the text width changes, make sure that the panel is large enough to fit. */
                            onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth + 16)
                        }
                        Button {
                            id: deleteButton

                            /* Take up 16 pixels on the right of the parent item. */
                            anchors.right: parent.right
                            width: 16

                            text: "X"

                            /* TODO - Maybe this should confirm? IDK... */
                            onClicked: FolderListing.deleteBookmark(modelData)
                        }
                    }
                }

                Button {
                    anchors {
                        left: drivePanelColumn.left
                        right: drivePanelColumn.right
                    }
                    text: "Add Bookmark"

                    onClicked: FolderListing.addBookmark(root.model.folder)

                    /* Whenever the text width changes, make sure that the panel is large enough to fit. */
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
                model: FolderListing
                delegate: fileComponent

                /* Include a plain scrollbar on the file view. */
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
                    font.family: googleMaterialFont.name
                    font.pixelSize: 32
                    text: "arrow_back"
                    
                    enabled: FolderListing.canGoBack
                    
                    onClicked: FolderListing.goBack()
                }
                
                ToolButton {
                    font.family: googleMaterialFont.name
                    font.pixelSize: 32
                    text: "arrow_forward"
                    
                    enabled: FolderListing.canGoForward
                    
                    onClicked: FolderListing.goForward()
                }
                
                ToolButton {
                    font.family: googleMaterialFont.name
                    font.pixelSize: 32
                    text: "arrow_upward"
                    
                    /* Don't go up if there is no up to go. */
                    enabled: FolderListing.canGoUp
                    
                    onClicked: FolderListing.goUp()
                }
                
                TextField {
                    id: pathText
                    
                    Layout.fillWidth: true
                    
                    onAccepted: {
                        if (FolderListing.dirExists(text))
                            FolderListing.currentDir = FolderListing.encodeURL(text)
                    }
                    
                    /* Android has some major issues with text input ATM, best to just leave read-only. */
                    readOnly: Qt.platform.os == "android"
                    
                    text: FolderListing.decodeURL(FolderListing.currentDir)
                }
                
                ToolButton {
                    font.family: googleMaterialFont.name
                    font.pixelSize: 32
                    text: "cancel"
                    
                    onClicked: root.cancel()
                }
            }
        }
    }

    /* A mouseArea covering everything to capture back/forward buttons. */
    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.ForwardButton | Qt.BackButton

        onClicked: {
            if (mouse.button == Qt.BackButton && FolderListing.canGoBack)
                FolderListing.goBack()
            if (mouse.button == Qt.ForwardButton && FolderListing.canGoForward)
                FolderListing.goForward()
        }
    }

    Shortcut {
        sequence: StandardKey.Back
        enabled: parent.visible && FolderListing.canGoBack
        context: Qt.ApplicationShortcut
        onActivated: FolderListing.goBack()
    }

    Shortcut {
        sequence: StandardKey.Forward
        enabled: parent.visible && FolderListing.canGoForward
        context: Qt.ApplicationShortcut
        onActivated: FolderListing.goForward()
    }
}

