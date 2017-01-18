import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Column {
    anchors.fill: parent

    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read. */
    property bool lockMouse: false
    property bool curvedScreen: false
    property real screenSize: 7
    property real screenDistance: 8
    property real screenHeight: 2

    function reset() {
        openVR_LockMouse.checked = lockMouse
        openVR_CurvedScreen.checked = curvedScreen
        openVR_ScreenSize.value = screenSize
        openVR_ScreenDistance.value = screenDistance
        openVR_ScreenHeight.value = screenHeight
    }
    function apply() {
        lockMouse = openVR_LockMouse.checked
        curvedScreen = openVR_CurvedScreen.checked
        screenSize = openVR_ScreenSize.value
        screenDistance = openVR_ScreenDistance.value
        screenHeight = openVR_ScreenHeight.value
    }

    CheckBox {
        id: openVR_LockMouse
        text: "Lock Mouse"

        checked: lockMouse
    }
    CheckBox {
        id: openVR_CurvedScreen
        text: "Curved Screen"

        checked: curvedScreen
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

            value: screenSize

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

            value: screenDistance

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

            value: screenHeight

            from: 1
            to: 40

            Layout.fillWidth: true
        }
    }
}
