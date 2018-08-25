import QtQuick 2.5
import QtQuick.Layouts 1.2
import DepthView 2.0
import QtQuick.Controls 2.1
import QtQuick.Window 2.2
import QtAV 1.6
import QtQuick.Controls.Material 2.2

Window {
    id: root

    Material.theme: Material.Dark

    FontLoader {
        id: googleMaterialFontLoader
        source: "qrc:/icons/material/MaterialIcons-Regular.ttf"
    }

    /* Default icon font settings. */
    property font googleMaterialFont:
        Qt.font({
                    family: googleMaterialFontLoader.name,
                    /* Use larger icons on screens with higher pixel density. */
                    pixelSize: Screen.pixelDensity < 4 ? 24 :
                              Screen.pixelDensity > 12 ? 96 : 32
                });
    property font uiTextFont:
        Qt.font({
                    /* Use larger text on screens with higher pixel density. */
                    pixelSize: Screen.pixelDensity > 12 ? 64 : 12
                });

    ImageViewer {
        id: image
        anchors.fill: parent

        onZoomChanged: bottomMenu.updateZoom()
    }

    TopMenu {
        id: topMenu

        /* Visible when the mouse is close or when the screen was recently touched. */
        forceOpen: fakeCursor.y < 128 || touchTimer.running
    }

    BottomMenu {
        id: bottomMenu

        /* Visible when the mouse is close or when the screen was recently touched.. */
        forceOpen: (root.contentItem.height - fakeCursor.y) < 128 || touchTimer.running
    }
    Dialog {
        id: aboutBox

        /* Anchors don't work on popups because they are appended to the window content item. */
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Label {
            id: aboutLabel

            text: qsTr("<h1><img source=\"qrc:/icons/logo.png\">DepthView %1
                  </h1>(%2)<p>DepthView is a basic application for viewing stereo 3D image and video files.</p>
                  <p>DepthView website: <a href=\"https://gitlab.com/chipgw/depthview2\">gitlab.com/chipgw/depthview2</a></p>
                  <p>Please report any bugs at: <a href=\"https://gitlab.com/chipgw/depthview2/issues\">gitlab.com/chipgw/depthview2/issues</a></p>")
                  .arg(DepthView.versionString()).arg(DepthView.gitVersion())

            /* Allow clicking links in the window. */
            onLinkActivated: Qt.openUrlExternally(link)

            textFormat: Text.RichText
        }

        standardButtons: Dialog.Close
    }
    Dialog {
        id: mediaInfoBox

        /* No anchors for some reason... */
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Label {
            id: mediaInfoLabel
            text: image.mediaInfo

            textFormat: Text.RichText
        }

        standardButtons: Dialog.Close
    }

    FileBrowser {
        id: fileBrowser

        width: parent.width
        height: parent.height

        visible: FolderListing.fileBrowserOpen

        /* Ensure video is paused when file browser is opened. */
        onOpened: if (image.isPlaying) image.playPause()
    }

    SettingsWindow {
        id: settingsPopup

        width: parent.width * 0.75
        height: parent.height * 0.75
        x: parent.width / 8
        y: parent.height / 8
    }

    Item {
        /* Start out off-screen so if the position doesn't get set it won't show up in the corner. */
        x: -childrenRect.width
        y: -childrenRect.height

        id: fakeCursor
        property bool isDot: false

        Image {
            source: parent.isDot ? "qrc:/images/vrcursor.png" : "qrc:/images/cursor.png"
            x: parent.isDot ? -width / 2 : 0
            y: parent.isDot ? -height / 2 : 0
        }

        /* Visible when the timer is running or when the UI is visible. */
        visible: mouseTimer.running || FolderListing.fileBrowserOpen || topMenu.state === "" || bottomMenu.state === ""

        Timer {
            id: mouseTimer
            interval: 4000
        }

        /* Use a separate timer for touchscreen events for showing toolbars instead of the cursor. */
        Timer {
            id: touchTimer
            interval: 2000
        }

        /* Must have a higher z than all popups, which are appended to the window's root item with a high z value. */
        z: 1200000
    }

    MouseArea {
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.BlankCursor

        /* Same situation as with fakeCursor, it needs to be above all popups. */
        z: 1200000
    }

    Connections {
        target: DepthView

        onMouseMoved: {
            fakeCursor.x = pos.x
            fakeCursor.y = pos.y

            /* Use the dot cursor when mouse events are synthesized from motion controller input. */
            fakeCursor.isDot = synthesized

            mouseTimer.restart()
        }

        onTouchEvent: touchTimer.restart()

        onFileInfo: mediaInfoBox.visible = !mediaInfoBox.visible

        onZoomActual: image.zoom = 1
        onZoomFit: image.zoom = -1

        onCancel: closePopups()
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: FolderListing.fileBrowserOpen = true
    }
    Shortcut {
        sequence: "S"
        onActivated: DepthView.takeSnapshot()
    }
    Shortcut {
        sequence: "Right"

        enabled: !FolderListing.fileBrowserOpen && !settingsPopup.visible

        onActivated: FolderListing.openNext()
    }
    Shortcut {
        sequence: "Left"

        enabled: !FolderListing.fileBrowserOpen && !settingsPopup.visible

        onActivated: FolderListing.openPrevious()
    }
    Shortcut {
        sequence: "P"

        enabled: FolderListing.currentFileIsVideo && !FolderListing.fileBrowserOpen && !settingsPopup.visible

        onActivated: image.playPause()
    }
    Shortcut {
        sequence: StandardKey.HelpContents
        onActivated: aboutBox.open()
    }
    Shortcut {
        sequence: StandardKey.FullScreen
        onActivated: DepthView.fullscreen = !DepthView.fullscreen
    }
    Shortcut {
        sequence: StandardKey.Preferences
        onActivated: settingsPopup.open()
    }

    function closePopups() {
        /* These cancel functions do more than just close the popup, so only call if the popup is open. */
        if (FolderListing.fileBrowserOpen)
            fileBrowser.cancel()
        if (settingsPopup.visible)
            settingsPopup.reject()

        aboutBox.close()
        mediaInfoBox.close()
        topMenu.closeMenus()
        bottomMenu.closeMenus()
    }

    Image {
        id: vrBackground
        source: VRManager.backgroundImage
        visible: false
        Connections {
            /* Give C++ access to the texture for the currently open image or video. */
            target: VRManager
            onBackgroundImageTargetChanged: VRManager.backgroundImageTarget = vrBackground
        }
    }
}
