import QtQuick 2.5
import Qt.labs.controls 1.0
import QtQuick.Layouts 1.1
import DepthView 2.0

Button {
    property bool onTop: false

    onClicked: popup.visible = !popup.visible

    property alias popupVisible: popup.visible

    /* Put children in the ColumnLayout. */
    default property alias contents: childrenTarget.children

    property Item root

    MouseArea {
        enabled: popup.visible

        anchors {
            fill: popup

            /* This way it will always fill the entire screen. (Unless the popup is offscreen, but does it really matter then?) */
            margins: -Math.max(root.width, root.height)
        }

        onClicked: popup.visible = false
    }

    MouseArea {
        /* Only a MouseArea in order to cover the above MouseArea. */
        id: hoverArea

        enabled: popup.visible

        anchors {
            fill: popup
            margins: -64
        }

        Connections {
            target: DepthView

            onMouseMoved: {
                if (!hoverArea.contains(hoverArea.mapFromItem(null, pos.x, pos.y)))
                    popup.visible = false
            }
        }
    }

    /* Show a simple rectangle behind mode dialog to ensure the text is always readable. */
    Rectangle {
        /* Get the background color from the system. */
        SystemPalette { id: palette; }
        color: palette.base

        id: popup

        anchors {
            top: onTop ? undefined : parent.bottom
            bottom: onTop ? parent.top : undefined
            left: parent.left
        }

        width: childrenRect.width + 8
        height: childrenRect.height + 16

        /* Hide by default and don't enable when hidden. */
        visible: false
        enabled: visible

        ColumnLayout {
            x: 4
            y: 8

            id: childrenTarget
        }
    }
}

