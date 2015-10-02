import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import DepthView 2.0

Button {
    property bool onTop: false

    onClicked: popup.visible = !popup.visible

    property alias popupVisible: popup.visible

    /* Put children in the ColumnLayout. */
    default property alias contents: childrenTarget.children

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

    Item {
        id: hoverArea
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
}

