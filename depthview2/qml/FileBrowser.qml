import QtQuick 2.5
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.0
import DepthView 2.0

Popup {
    id: root

    padding: 0

    property real cellWidth: 320
    property real cellHeight: 240

    property url startingFolder

    function cancel() {
        /* Reset to the folder that was active when the browser was first shown. */
        FolderListing.currentDir = startingFolder
        FolderListing.fileBrowserOpen = false;
    }

    onOpened: startingFolder = FolderListing.currentDir

    Component {
        id: fileComponent

        /* Item to have the rectangle padded inside. */
        Item {
            id: wrapper
            /* So that it is the same as the GridView. */
            width: root.cellWidth
            height: root.cellHeight

            function accept() {
                if (fileIsDir)
                    FolderListing.currentDir = fileURL
                else
                    FolderListing.openFile(fileURL)
            }

            /* Border/highlight rectangle. */
            Rectangle {
                id: fileRect
                anchors { fill: parent; margins: 8 }
                radius: 4
                color: (wrapper.GridView.isCurrentItem || mouseArea.containsMouse) ? "#44444488" : "transparent"
                border { color: "grey"; width: 4 }

                MouseArea {
                    id: mouseArea
                    anchors { fill: parent; margins: 4 }

                    onClicked: accept()

                    ToolTip {
                        y: fakeCursor.height
                        visible: mouseArea.containsMouse

                        text: "Type: " + (fileIsDir ? "Folder" : (fileIsVideo ? "Video" : "Image") +
                              "<br>Size: " + FolderListing.bytesToString(fileSize)) +
                              "<br>Created: " + fileCreated +
                              "<br>" + FolderListing.decodeURL(fileURL)

                        parent: fakeCursor
                    }

                    hoverEnabled: true

                    /* Item to center the thumbnail inside. */
                    Item {
                        /* Fill the parent except for on the bottom where the text is. */
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            bottom: fileNameText.top
                        }

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: thumb.status === Image.Loading
                        }

                        /* 3D thumbnails! */
                        StereoImage {
                            id: thumb

                            anchors.centerIn: parent

                            imageMode: fileStereoMode

                            /* If it is a directory use a thumbnail from qrc. Otherwise the file should be an image itself. */
                            source: fileIsDir ? "qrc:/images/folder.pns" :
                                    fileIsVideo ? "image://video/" + FolderListing.decodeURL(fileURL) : fileURL

                            /* Images on the filesystem should be loaded asynchronously. */
                            asynchronous: !fileIsDir;

                            /* The image should only be stored at the needed size. */
                            sourceSize: Qt.size((imageMode === SourceMode.SidebySide) ? parent.width * 2 : parent.width,
                                                (imageMode === SourceMode.TopBottom) ? parent.height * 2 : parent.height)
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
                    font: uiTextFont
                }

                Repeater {
                    model: FolderListing.storageDevicePaths

                    Item {
                        width: drivePanel.width
                        height: button.implicitHeight
                        Button {
                            id: button
                            anchors.fill: parent
                            text: data[1]

                            /* Drive info is provided as "<path>;<display name>". */
                            property variant data: modelData.split(';')

                            onClicked: FolderListing.currentDir = FolderListing.encodeURL(data[0])

                            /* Whenever the text width changes, make sure that the panel is large enough to fit. */
                            onImplicitWidthChanged: drivePanel.width = Math.max(drivePanel.width, implicitWidth)
                        }

                        ToolTip {
                            y: fakeCursor.height

                            /* Don't use button.hovered because it breaks when the file browser closes. */
                            visible: button.contains(button.mapFromItem(null, fakeCursor.x, fakeCursor.y)) && root.visible

                            text: button.data[0] + "<br>" + FolderListing.bytesToString(button.data[2])

                            parent: fakeCursor
                        }
                    }
                }

                Label {
                    padding: 8
                    text: "Bookmarks:"
                    font: uiTextFont
                }

                Repeater {
                    model: FolderListing.bookmarks

                    Item {
                        height: childrenRect.height
                        width: drivePanel.width

                        Button {
                            id: bookmarkButton

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
                        ToolTip {
                            y: fakeCursor.height

                            /* Don't use button.hovered because it breaks when the file browser closes. */
                            visible: bookmarkButton.contains(bookmarkButton.mapFromItem(null, fakeCursor.x, fakeCursor.y)) && root.visible

                            text: FolderListing.decodeURL(modelData)

                            parent: fakeCursor
                        }
                    }
                }

                Button {
                    anchors {
                        left: drivePanelColumn.left
                        right: drivePanelColumn.right
                    }
                    text: "Add Bookmark"

                    onClicked: FolderListing.addBookmark(FolderListing.currentDir)

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
                id: grid
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
                    font: googleMaterialFont
                    /* "arrow_back" */
                    text: "\ue5c4"
                    
                    enabled: FolderListing.canGoBack
                    
                    onClicked: FolderListing.goBack()
                }
                
                ToolButton {
                    font: googleMaterialFont
                    /* "arrow_forward" */
                    text: "\ue5c8"
                    
                    enabled: FolderListing.canGoForward
                    
                    onClicked: FolderListing.goForward()
                }
                
                ToolButton {
                    font: googleMaterialFont
                    /* "arrow_upward" */
                    text: "\ue5d8"
                    
                    /* Don't go up if there is no up to go. */
                    enabled: FolderListing.canGoUp
                    
                    onClicked: FolderListing.goUp()
                }
                
                TextField {
                    id: pathText

                    font: uiTextFont
                    Layout.fillWidth: true
                    
                    onAccepted: {
                        if (FolderListing.dirExists(text))
                            FolderListing.currentDir = FolderListing.encodeURL(text)
                    }
                    
                    /* Android has some major issues with text input ATM, best to just leave read-only. */
                    readOnly: Qt.platform.os === "android"
                    
                    text: FolderListing.decodeURL(FolderListing.currentDir)
                }
                
                ToolButton {
                    font: googleMaterialFont
                    /* "cancel" */
                    text: "\ue5c9"
                    
                    onClicked: root.cancel()
                }
            }
        }
    }

    /* A MouseArea covering everything to capture back/forward buttons. */
    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.ForwardButton | Qt.BackButton

        onClicked: {
            if (mouse.button === Qt.BackButton && FolderListing.canGoBack)
                FolderListing.goBack()
            if (mouse.button === Qt.ForwardButton && FolderListing.canGoForward)
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

    Connections {
        target: DepthView

        onAccept: grid.currentItem.accept();
        onUp: grid.moveCurrentIndexUp()
        onDown: grid.moveCurrentIndexDown()
        onLeft: grid.moveCurrentIndexLeft()
        onRight: grid.moveCurrentIndexRight()
    }
    Connections {
        target: FolderListing

        onCurrentDirChanged: grid.currentIndex = -1
    }
}

