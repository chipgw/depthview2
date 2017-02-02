import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Column {
    width: parent.width

    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read. */
    property bool lockMouse: false
    property real screenCurve: 0
    property real screenSize: 7
    property real screenDistance: 8
    property real screenHeight: 2

    function reset() {
        openVR_LockMouse.checked = lockMouse
        openVR_ScreenCurve.value = screenCurve
        openVR_ScreenSize.value = screenSize
        openVR_ScreenDistance.value = screenDistance
        openVR_ScreenHeight.value = screenHeight
    }
    function apply() {
        lockMouse = openVR_LockMouse.checked
        screenCurve = openVR_ScreenCurve.value
        screenSize = openVR_ScreenSize.value
        screenDistance = openVR_ScreenDistance.value
        screenHeight = openVR_ScreenHeight.value
    }

    CheckBox {
        id: openVR_LockMouse
        text: "Lock Mouse"

        checked: lockMouse
    }
    RowLayout {
        width: parent.width
        Label {
            text: "Screen Curviness"
        }
        Slider {
            id: openVR_ScreenCurve

            value: screenCurve

            from: 0
            to: 1

            Layout.fillWidth: true
        }
    }

    RowLayout {
        width: parent.width
        Label {
            text: "Screen Size"
            anchors.verticalCenter: parent.verticalCenter
        }
        Slider {
            id: openVR_ScreenSize

            value: screenSize

            from: 1
            to: openVR_ScreenDistance.value * 2

            Layout.fillWidth: true
        }
    }

    RowLayout {
        width: parent.width
        Label {
            text: "Screen Distance"
            anchors.verticalCenter: parent.verticalCenter
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
            anchors.verticalCenter: parent.verticalCenter
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
