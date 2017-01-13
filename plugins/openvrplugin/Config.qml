import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Column {
    anchors.fill: parent

    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read. */
    property alias lockMouse: openVR_LockMouse.checked
    property alias curvedScreen: openVR_CurvedScreen.checked
    property alias screenSize: openVR_ScreenSize.value
    property alias screenDistance: openVR_ScreenDistance.value
    property alias screenHeight: openVR_ScreenHeight.value

    CheckBox {
        id: openVR_LockMouse
        text: "Lock Mouse"
    }
    CheckBox {
        id: openVR_CurvedScreen
        text: "Curved Screen"
    }

    RowLayout {
        width: parent.width
        Label {
            text: "Screen Size"
            height: parent.height
            verticalAlignment: Text.AlignVCenter
        }
        Slider {
            id: openVR_ScreenSize

            value: 7

            from: 1
            to: screenDistance * 2

            Layout.fillWidth: true
        }
    }

    RowLayout {
        width: parent.width
        Label {
            text: "Screen Distance"
            height: parent.height
            verticalAlignment: Text.AlignVCenter
        }
        Slider {
            id: openVR_ScreenDistance

            value: 8

            from: 1
            to: 100

            Layout.fillWidth: true
        }
    }

    RowLayout {
        width: parent.width
        Label {
            text: "Screen Height"
            height: parent.height
            verticalAlignment: Text.AlignVCenter
        }

        Slider {
            id: openVR_ScreenHeight

            value: 2

            from: 1
            to: 40

            Layout.fillWidth: true
        }
    }
}
